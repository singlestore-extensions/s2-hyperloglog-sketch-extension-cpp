#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <limits>
#include <extension.h>

const int DEFAULT_LG_K = 12;
const int MIN_LG_K = 4;
const int MAX_LG_K = 21;

class Extension {
private:
    int lgK;
    int k;
    std::vector<uint8_t> buckets;
    int64_t numNonZero;
    bool isDenseMode;
    bool valid;

    static const int VALUE_BITS;
    static const int HLL_BYTE_ARR_START;
    static const uint8_t PREAMBLE_INTS_BYTE;
    static const uint8_t SER_VER_BYTE;
    static const uint8_t FAMILY_BYTE;
    static const uint8_t EMPTY_FLAG_MASK;
    static const uint8_t COMPACT_FLAG_MASK;
    static const uint8_t OUT_OF_ORDER_FLAG_MASK;
    static const uint8_t FULL_SIZE_FLAG_MASK;

    static void packBits(const std::vector<uint8_t>& input, std::vector<uint8_t>& output, 
                        size_t startIdx, size_t numItems, int srcBits) {
        int srcMask = (1 << srcBits) - 1;
        int dstBitPos = 0;
        size_t dstBytePos = startIdx;

        for (size_t i = 0; i < numItems; i++) {
            int value = input[i] & srcMask;
            int bitsRemaining = srcBits;

            while (bitsRemaining > 0) {
                int bitsAvailableInByte = 8 - dstBitPos;
                int bitsToWrite = std::min(bitsAvailableInByte, bitsRemaining);
                int shift = bitsRemaining - bitsToWrite;

                int fragment = (value >> shift) & ((1 << bitsToWrite) - 1);
                output[dstBytePos] |= (fragment << (bitsAvailableInByte - bitsToWrite));

                bitsRemaining -= bitsToWrite;
                dstBitPos += bitsToWrite;

                if (dstBitPos == 8) {
                    dstBitPos = 0;
                    dstBytePos++;
                }
            }
        }
    }

    static void unpackBits(const std::vector<uint8_t>& input, std::vector<uint8_t>& output,
                        size_t startIdx, size_t numItems, int dstBits) {
        int srcBitPos = 0;
        size_t srcBytePos = startIdx;
        int dstMask = (1 << dstBits) - 1;

        for (size_t i = 0; i < numItems; i++) {
            int value = 0;
            int bitsNeeded = dstBits;

            while (bitsNeeded > 0) {
                if (srcBytePos >= input.size()) {
                    output.clear();
                    return;
                }
                int bitsAvailable = 8 - srcBitPos;
                int bitsToRead = std::min(bitsAvailable, bitsNeeded);

                int fragment = (input[srcBytePos] >> (bitsAvailable - bitsToRead)) & 
                            ((1 << bitsToRead) - 1);
                value = (value << bitsToRead) | fragment;

                bitsNeeded -= bitsToRead;
                srcBitPos += bitsToRead;

                if (srcBitPos == 8) {
                    srcBitPos = 0;
                    srcBytePos++;
                }
            }

            output[i] = value & dstMask;
        }
    }

    void couponUpdate(uint32_t coupon) {
        int slotNo = coupon >> VALUE_BITS;
        uint8_t newValue = coupon & ((1 << VALUE_BITS) - 1);

        if (!isDenseMode) {
            uint8_t& bucket = buckets[slotNo];
            if (bucket < newValue) {
                if (bucket == 0) {
                    numNonZero++;
                }
                bucket = newValue;
            }
            if (numNonZero > k / 16) {
                toDense();
            }
        } else {
            if (buckets[slotNo] < newValue) {
                buckets[slotNo] = newValue;
            }
        }
    }

public:
    Extension() : lgK(DEFAULT_LG_K),
                  k(1 << DEFAULT_LG_K),
                  buckets(k, 0),
                  numNonZero(0),
                  isDenseMode(false),
                  valid(true) {}

    Extension(int lgK) : lgK(std::max(MIN_LG_K, std::min(MAX_LG_K, lgK))),
                         k(1 << this->lgK),
                         buckets(k, 0),
                         numNonZero(0),
                         isDenseMode(false),
                         valid(true) {}

    Extension(const Extension& other) = default;

    Extension(Extension&& other) noexcept = default;

    Extension& operator=(const Extension& other) = default;

    Extension& operator=(Extension&& other) noexcept = default;

    bool isValid() const { return valid; }

    static uint64_t hash(const uint8_t* key, size_t len) {
        const uint64_t m = 0xc6a4a7935bd1e995ULL;
        const int r = 47;

        uint64_t h = 0x8445d61a4e774912 ^ (len * m);

        const uint8_t* data = key;
        const uint8_t* end = data + (len & ~7);

        while (data != end) {
            uint64_t k;
            memcpy(&k, data, sizeof(uint64_t));

            k *= m;
            k ^= k >> r;
            k *= m;

            h ^= k;
            h *= m;

            data += 8;
        }

        switch (len & 7) {
        case 7: h ^= uint64_t(data[6]) << 48;
        case 6: h ^= uint64_t(data[5]) << 40;
        case 5: h ^= uint64_t(data[4]) << 32;
        case 4: h ^= uint64_t(data[3]) << 24;
        case 3: h ^= uint64_t(data[2]) << 16;
        case 2: h ^= uint64_t(data[1]) << 8;
        case 1: h ^= uint64_t(data[0]);
                h *= m;
        };

        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return h;
    }

    void updateWithHash(uint64_t hashValue) {
        int slotNo = hashValue >> (64 - lgK);
        uint64_t w = hashValue << lgK;
        int rank;

        if (w == 0) {
            rank = (64 - lgK) + 1;
        } else {
            rank = __builtin_clzll(w) + 1;
            if (rank > (64 - lgK)) rank = 64 - lgK;
        }

        uint32_t coupon = (slotNo << VALUE_BITS) | rank;
        couponUpdate(coupon);
    }

    void update(const uint8_t* key, size_t len) {
        updateWithHash(hash(key, len));
    }

    void toDense() {
        if (!isDenseMode) {
            isDenseMode = true;
            numNonZero = std::count_if(buckets.begin(), buckets.end(), [](uint8_t val) { return val != 0; });
        }
    }

    double estimate() const {
        double sum = 0.0;
        int zeros = 0;
        for (int i = 0; i < k; ++i) {
            uint8_t val = buckets[i];
            if (val == 0) {
                zeros++;
            }
            sum += 1.0 / (1ULL << val);
        }

        double alpha;
        switch (k) {
            case 16: alpha = 0.673; break;
            case 32: alpha = 0.697; break;
            case 64: alpha = 0.709; break;
            default: alpha = 0.7213 / (1.0 + 1.079 / k); break;
        }

        double estimate = alpha * k * k / sum;

        if (estimate <= 2.5 * k) {
            if (zeros != 0) {
                estimate = k * log(static_cast<double>(k) / zeros);
            }
        }
        else if (estimate > (1.0 / 30.0) * (1ULL << 32)) {
            estimate = -(1ULL << 32) * log(1.0 - (estimate / (1ULL << 32)));
        }

        return estimate;
    }

    static void writeVarInt(std::vector<uint8_t>& buffer, uint32_t value) {
        while (value >= 0x80) {
            buffer.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
            value >>= 7;
        }
        buffer.push_back(static_cast<uint8_t>(value & 0x7F));
    }

    static bool readVarInt(const std::vector<uint8_t>& data, size_t& offset, uint32_t& value) {
        value = 0;
        int shift = 0;
        while (offset < data.size()) {
            uint8_t byte = data[offset++];
            value |= (uint32_t)(byte & 0x7F) << shift;
            if ((byte & 0x80) == 0) return true;
            shift += 7;
            if (shift > 28) {
                return false;
            }
        }
        return false;
    }

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> result;
        result.reserve(isDenseMode ? 5 + k : 5 + 3 * numNonZero);

        result.push_back(PREAMBLE_INTS_BYTE);
        result.push_back(SER_VER_BYTE);
        result.push_back(FAMILY_BYTE);

        uint8_t flags = 0;
        if (isDenseMode) flags |= FULL_SIZE_FLAG_MASK;
        result.push_back(flags);

        result.push_back(static_cast<uint8_t>(lgK));

        if (!isDenseMode) {
            writeVarInt(result, static_cast<uint32_t>(numNonZero));

            for (int i = 0; i < k; ++i) {
                if (buckets[i] != 0) {
                    writeVarInt(result, static_cast<uint32_t>(i));
                    result.push_back(buckets[i]);
                }
            }
        } else {
            result.insert(result.end(), buckets.begin(), buckets.end());
        }
        return result;
    }

    std::vector<uint8_t> serialize_compact() const {
        std::vector<uint8_t> result;
        result.reserve(8);

        result.push_back(PREAMBLE_INTS_BYTE);
        result.push_back(SER_VER_BYTE);
        result.push_back(FAMILY_BYTE);

        uint8_t flags = 0;
        if (isDenseMode) flags |= FULL_SIZE_FLAG_MASK;
        flags |= COMPACT_FLAG_MASK;
        result.push_back(flags);

        result.push_back(static_cast<uint8_t>(lgK));

        if (!isDenseMode) {
            writeVarInt(result, static_cast<uint32_t>(numNonZero));
            
            std::vector<uint32_t> pairs;
            for (int i = 0; i < k; ++i) {
                if (buckets[i] != 0) {
                    uint32_t pair = (static_cast<uint32_t>(i) << VALUE_BITS) | buckets[i];
                    pairs.push_back(pair);
                }
            }
            
            std::sort(pairs.begin(), pairs.end());
            
            for (uint32_t pair : pairs) {
                writeVarInt(result, pair);
            }
        } else {
            size_t numBytes = (k * VALUE_BITS + 7) / 8;
            size_t currentSize = result.size();
            result.resize(currentSize + numBytes, 0);

            packBits(buckets, result, currentSize, k, VALUE_BITS);
        }

        return result;
    }

    static Extension deserialize(const std::vector<uint8_t>& data) {
        Extension hll;
        if (data.size() < 5) {
            hll.valid = false;
            return hll;
        }

        size_t offset = 0;

        uint8_t preambleInts = data[offset++];
        uint8_t serVer = data[offset++];
        uint8_t family = data[offset++];
        uint8_t flags = data[offset++];
        int lgK = data[offset++];
        
        if (preambleInts != PREAMBLE_INTS_BYTE || serVer != SER_VER_BYTE || 
            family != FAMILY_BYTE || lgK < MIN_LG_K || lgK > MAX_LG_K) {
            hll.valid = false;
            return hll;
        }

        bool isDenseMode = (flags & FULL_SIZE_FLAG_MASK) != 0;
        bool isCompact = (flags & COMPACT_FLAG_MASK) != 0;

        hll.lgK = lgK;
        hll.k = 1 << lgK;
        hll.isDenseMode = isDenseMode;
        hll.buckets.resize(hll.k, 0);

        if (!isDenseMode) {
            uint32_t numNonZero;
            if (!readVarInt(data, offset, numNonZero)) {
                hll.valid = false;
                return hll;
            }
            hll.numNonZero = numNonZero;

            if (isCompact) {
                for (uint32_t i = 0; i < numNonZero; ++i) {
                    uint32_t pair;
                    if (!readVarInt(data, offset, pair)) {
                        hll.valid = false;
                        return hll;
                    }
                    uint32_t index = pair >> VALUE_BITS;
                    uint8_t value = pair & ((1 << VALUE_BITS) - 1);

                    if (index >= hll.k) {
                        hll.valid = false;
                        return hll;
                    }
                    hll.buckets[index] = value;
                }
            } else {
                while (offset < data.size()) {
                    uint32_t index;
                    if (!readVarInt(data, offset, index)) {
                        hll.valid = false;
                        return hll;
                    }

                    if (offset >= data.size()) {
                        hll.valid = false;
                        return hll;
                    }

                    uint8_t value = data[offset++];

                    if (index >= hll.k) {
                        hll.valid = false;
                        return hll;
                    }

                    hll.buckets[index] = value;
                }
            }
        } else {
            if (isCompact) {
                unpackBits(data, hll.buckets, offset, hll.k, VALUE_BITS);
            } else {
                if (data.size() != offset + hll.k) {
                    hll.valid = false;
                    return hll;
                }
                hll.buckets.assign(data.begin() + offset, data.end());
            }
        }

        hll.valid = true;
        return hll;
    }

    void merge(const Extension& other) {
        if (lgK != other.lgK) return;

        if (!isDenseMode && !other.isDenseMode) {
            for (int i = 0; i < k; i++) {
                if (other.buckets[i] > buckets[i]) {
                    couponUpdate((i << VALUE_BITS) | other.buckets[i]);
                }
            }
        } else {
            if (!isDenseMode) toDense();
            for (int i = 0; i < k; i++) {
                uint8_t newValue = std::max(buckets[i], other.buckets[i]);
                if (newValue > buckets[i]) {
                    couponUpdate((i << VALUE_BITS) | newValue);
                }
            }
        }
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "HyperLogLog Sketch:\n  LgK: " << lgK << "\n  K: " << k
            << "\n  Mode: " << (isDenseMode ? "Dense" : "Sparse")
            << "\n  Estimated cardinality: " << std::llround(estimate());
        return oss.str();
    }

    bool isSparse() const { return !isDenseMode; }
    bool isDense() const { return isDenseMode; }
};

const int Extension::VALUE_BITS = 7;
const int Extension::HLL_BYTE_ARR_START = 40;
const uint8_t Extension::PREAMBLE_INTS_BYTE = 8;
const uint8_t Extension::SER_VER_BYTE = 1;
const uint8_t Extension::FAMILY_BYTE = 1;
const uint8_t Extension::EMPTY_FLAG_MASK = 4;
const uint8_t Extension::COMPACT_FLAG_MASK = 8;
const uint8_t Extension::OUT_OF_ORDER_FLAG_MASK = 16;
const uint8_t Extension::FULL_SIZE_FLAG_MASK = 32;

extern "C" {
    extension_state_t extension_hll_empty() {
        return reinterpret_cast<extension_state_t>(new Extension());
    }

    void extension_hll_free(extension_state_t state) {
        if (state != 0) {
            Extension* hll = reinterpret_cast<Extension*>(state);
            delete hll;
        }
    }

    extension_state_t extension_hll_add(extension_state_t state, extension_list_u8_t* input) {
        if (input == nullptr || input->len == 0 || input->ptr == nullptr) {
            return state;
        }
        Extension* hll = reinterpret_cast<Extension*>(state);
        if (hll == nullptr) {
            hll = new Extension();
        }
        hll->update(input->ptr, input->len);
        return reinterpret_cast<extension_state_t>(hll);
    }

    extension_state_t extension_hll_add_emptyisnull(extension_state_t state, extension_list_u8_t* input) {
        return extension_hll_add(state, input);
    }

    extension_state_t extension_hll_add_hash(extension_state_t state, uint64_t input) {
        Extension* hll = reinterpret_cast<Extension*>(state);
        if (hll == nullptr) {
            hll = new Extension();
        }
        hll->updateWithHash(input);
        return reinterpret_cast<extension_state_t>(hll);
    }

    extension_state_t extension_hll_add_hash_emptyisnull(extension_state_t state, uint64_t input) {
        return extension_hll_add_hash(state, input);
    }

    uint64_t extension_hll_hash(extension_list_u8_t* data) {
        if (data == nullptr || data->len == 0 || data->ptr == nullptr) {
            return 0;
        }
        return Extension::hash(data->ptr, data->len);
    }

    uint64_t extension_hll_hash_emptyisnull(extension_list_u8_t* data) {
        return extension_hll_hash(data);
    }

    extension_state_t extension_hll_union_merge(extension_state_t left, extension_state_t right) {
        if (left == 0 && right == 0) {
            Extension* new_hll = new Extension();
            return reinterpret_cast<extension_state_t>(new_hll);
        } else if (left == 0) {
            return right;
        } else if (right == 0) {
            return left;
        } else {
            Extension* hll_left = reinterpret_cast<Extension*>(left);
            Extension* hll_right = reinterpret_cast<Extension*>(right);

            hll_left->merge(*hll_right);
            delete hll_right;
            return left;
        }
    }

    extension_state_t extension_hll_to_dense(extension_state_t state) {
        if (state == 0) {
            return reinterpret_cast<extension_state_t>(new Extension());
        }
        Extension* hll = reinterpret_cast<Extension*>(state);
        if (hll->isSparse()) {
            hll->toDense();
        }
        return state;
    }

    double extension_hll_cardinality(extension_list_u8_t* data) {
        if (data == nullptr || data->len == 0 || data->ptr == nullptr) {
            return 0.0;
        }
        std::vector<uint8_t> vec(data->ptr, data->ptr + data->len);
        Extension hll = Extension::deserialize(vec);
        if (!hll.isValid()) {
            return 0.0;
        }
        return hll.estimate();
    }

    double extension_hll_cardinality_emptyisnull(extension_list_u8_t* data) {
        return extension_hll_cardinality(data);
    }

    void extension_hll_union(extension_list_u8_t* left, extension_list_u8_t* right, extension_list_u8_t* ret0) {
        if (left == nullptr || right == nullptr || ret0 == nullptr) return;

        std::vector<uint8_t> vec_left(left->ptr, left->ptr + left->len);
        std::vector<uint8_t> vec_right(right->ptr, right->ptr + right->len);
        
        Extension hll_left = Extension::deserialize(vec_left);
        Extension hll_right = Extension::deserialize(vec_right);

        hll_left.merge(hll_right);

        std::vector<uint8_t> result = hll_left.serialize();
        ret0->ptr = (uint8_t*)malloc(result.size());
        ret0->len = result.size();
        memcpy(ret0->ptr, result.data(), result.size());
    }

    void extension_hll_union_emptyisnull(extension_list_u8_t* left, extension_list_u8_t* right, extension_list_u8_t* ret0) {
        extension_hll_union(left, right, ret0);
    }

    void extension_hll_print(extension_list_u8_t* data, extension_string_t* ret0) {
        if (data == nullptr || ret0 == nullptr) return;

        std::vector<uint8_t> vec(data->ptr, data->ptr + data->len);
        Extension hll = Extension::deserialize(vec);

        std::string result = hll.toString();
        ret0->ptr = (char*)malloc(result.size() + 1);
        ret0->len = result.size();
        memcpy(ret0->ptr, result.c_str(), result.size() + 1);
    }

    void extension_hll_print_emptyisnull(extension_list_u8_t* data, extension_string_t* ret0) {
        extension_hll_print(data, ret0);
    }

    extension_state_t extension_hll_union_agg(extension_state_t state, extension_list_u8_t* input) {
        Extension* hll_state = reinterpret_cast<Extension*>(state);
        if (input == nullptr || input->len == 0 || input->ptr == nullptr) {
            return state;
        }
        Extension hll_input = Extension::deserialize(
            std::vector<uint8_t>(input->ptr, input->ptr + input->len));
        if (!hll_input.isValid()) {
            return state;
        }
        if (hll_state == nullptr) {
            hll_state = new Extension(hll_input);
            return reinterpret_cast<extension_state_t>(hll_state);
        } else {
            hll_state->merge(hll_input);
            return state;
        }
    }

    extension_state_t extension_hll_union_agg_emptyisnull(extension_state_t state, extension_list_u8_t* input) {
        return extension_hll_union_agg(state, input);
    }

    void extension_hll_serialize(extension_state_t state, extension_list_u8_t* ret0) {
        if (state == 0 || ret0 == nullptr) {
            if (ret0) {
                ret0->ptr = nullptr;
                ret0->len = 0;
            }
            return;
        }
        Extension* hll = reinterpret_cast<Extension*>(state);
        std::vector<uint8_t> result = hll->serialize();
        ret0->ptr = (uint8_t*)malloc(result.size());
        ret0->len = result.size();
        memcpy(ret0->ptr, result.data(), result.size());
    }

    void extension_hll_serialize_compact(extension_state_t state, extension_list_u8_t* ret0) {
        if (state == 0 || ret0 == nullptr) {
            if (ret0) {
                ret0->ptr = nullptr;
                ret0->len = 0;
            }
            return;
        }
        Extension* hll = reinterpret_cast<Extension*>(state);
        std::vector<uint8_t> result = hll->serialize_compact();
        ret0->ptr = (uint8_t*)malloc(result.size());
        ret0->len = result.size();
        memcpy(ret0->ptr, result.data(), result.size());
    }

    extension_state_t extension_hll_deserialize(extension_list_u8_t* data) {
        if (data == nullptr || data->len == 0 || data->ptr == nullptr) {
            return 0;
        }
        std::vector<uint8_t> vec(data->ptr, data->ptr + data->len);
        Extension hll = Extension::deserialize(vec);
        if (!hll.isValid()) {
            return 0;
        }
        Extension* hll_ptr = new Extension(std::move(hll));
        return reinterpret_cast<extension_state_t>(hll_ptr);
    }

    uint32_t extension_hll_is_sparse(extension_state_t state) {
        if (state == 0) return 1;
        Extension* hll = reinterpret_cast<Extension*>(state);
        return hll->isSparse() ? 1 : 0;
    }

    uint32_t extension_hll_is_dense(extension_state_t state) {
        if (state == 0) return 0;
        Extension* hll = reinterpret_cast<Extension*>(state);
        return hll->isDense() ? 1 : 0;
    }

}