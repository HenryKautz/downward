#ifndef ALGORITHMS_HASH_SET_H
#define ALGORITHMS_HASH_SET_H

#include "../utils/collections.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace algorithms {
/*
  Hash set using a single vector for storing integer keys.

  During key insertion we use ideas from hopscotch hashing to ensure
  that each key is at most H buckets away from its ideal bucket. This
  ensures constant lookup times.

  The hash set allows storing only non-negative integers.
*/
template <typename Hasher, typename Equal>
class IntHashSet {
    using KeyType = int;
    using HashType = unsigned int;

    class Bucket {
        static const KeyType empty_bucket = -1;
public:
        KeyType key;
        HashType hash;

        Bucket()
            : key(empty_bucket),
              hash(0) {
        }

        Bucket(const KeyType &key, const HashType &hash)
            : key(key),
              hash(hash) {
        }

        bool empty() const {
            return key == empty_bucket;
        }
    };

    Hasher hasher;
    Equal equal;
    std::vector<Bucket> buckets;
    int num_entries;
    int max_distance;

    void enlarge() {
        reserve(buckets.size() * 2);
    }

    int wrap_unsigned(HashType hash) const {
        /* Note: If we restricted the number of buckets to powers of 2,
           we could use "i % 2^n = i & (2^n – 1)" to speed this up. */
        assert(buckets.size() != 0);
        return hash % buckets.size();
    }

    int wrap_signed(int i) const {
        if (i < 0) {
            i += buckets.size();
        }
        return wrap_unsigned(i);
    }

    int get_distance(int left_index, int right_index) const {
        assert(utils::in_bounds(left_index, buckets));
        assert(utils::in_bounds(right_index, buckets));
        if (right_index >= left_index) {
            return right_index - left_index;
        } else {
            return buckets.size() - left_index + right_index;
        }
    }

    int find_next_free_bucket_index(int index) const {
        assert(num_entries < buckets.size());
        assert(utils::in_bounds(index, buckets));
        while (true) {
            if (buckets[index].empty()) {
                return index;
            }
            index = wrap_signed(++index);
        }
    }

public:
    IntHashSet(int capacity = 1, int max_distance = 32)
        : buckets(capacity),
          num_entries(0),
          max_distance(max_distance) {
        assert(capacity >= 1);
        assert(max_distance >= 0);
    }

    int size() const {
        return num_entries;
    }

    /*
      Ensure that each key is at most H buckets away from its ideal
      bucket by moving the closest free bucket towards the ideal
      bucket. If this can't be achieved, we resize the vector, reinsert
      the old keys and try inserting the new key again.
    */
    bool insert(const KeyType &key) {
        if (contains(key)) {
            return false;
        }

        assert(num_entries <= buckets.size());
        if (num_entries == buckets.size()) {
            enlarge();
        }
        assert(num_entries < buckets.size());

        HashType hash = hasher(key);
        int ideal_index = wrap_unsigned(hash);
        std::cout << "hash(" << key << ") = " << hash
                  << " -> ideal index: " << ideal_index << std::endl;

        // Find next free bucket.
        int free_index = find_next_free_bucket_index(ideal_index);
        std::cout << "free index: " << free_index << std::endl;

        // Move the free bucket towards the ideal index.
        while (get_distance(ideal_index, free_index) >= max_distance) {
            bool swapped = false;
            for (int offset = max_distance - 1; offset >= 1; --offset) {
                int candidate_index = wrap_signed(free_index - offset);
                HashType candidate_hash = buckets[candidate_index].hash;
                int candidate_ideal_index = wrap_unsigned(candidate_hash);
                std::cout << "candidate index: " << candidate_index
                          << ", ideal index: " << candidate_ideal_index
                          << ", dist: " << get_distance(candidate_ideal_index, free_index)
                          << std::endl;
                if (get_distance(candidate_ideal_index, free_index) < max_distance) {
                    // Candidate can be swapped.
                    std::swap(buckets[candidate_index], buckets[free_index]);
                    free_index = candidate_index;
                    std::cout << "free index: " << free_index << std::endl;
                    swapped = true;
                    break;
                }
            }
            if (!swapped) {
                /* Free bucket could not be moved close enough.
                   -> Enlarge and try inserting again. */
                enlarge();
                return insert(key);
            }
        }
        std::cout << "used index: " << free_index << std::endl;
        assert(utils::in_bounds(free_index, buckets));
        assert(buckets[free_index].empty());
        buckets[free_index] = Bucket(key, hash);
        ++num_entries;
        return true;
    }

    bool contains(const KeyType &key) const {
        HashType hash = hasher(key);
        int ideal_index = wrap_unsigned(hash);
        for (int i = 0; i < max_distance; ++i) {
            int index = wrap_signed(ideal_index + i);
            const Bucket &bucket = buckets[index];
            if (!bucket.empty() && bucket.hash == hash && equal(bucket.key, key)) {
                return true;
            }
        }
        return false;
    }

    void reserve(int new_capacity) {
        std::cout << "before reserve: ";
        dump();
        std::vector<Bucket> old_buckets = std::move(buckets);
        assert(buckets.empty());
        num_entries = 0;
        buckets.resize(new_capacity);
        /* Sort old buckets by decreasing ideal positions to obtain a
           better layout while reinserting them. */
        sort(old_buckets.begin(), old_buckets.end(),
             [this](const Bucket &b1, const Bucket &b2) {
                return wrap_unsigned(b1.hash) > wrap_unsigned(b2.hash);
            });
        for (Bucket &bucket : old_buckets) {
            std::cout << "old: " << bucket.key
                      << " hash: " << wrap_unsigned(bucket.hash) << std::endl;
            if (!bucket.empty()) {
                insert(bucket.key);
            }
        }
        std::cout << "after reserve: ";
        dump();
        std::cout << "capacity: " << buckets.size() << std::endl;
    }

    void dump() const {
        std::cout << "[";
        for (int i = 0; i < buckets.size(); ++i) {
            const Bucket &bucket = buckets[i];
            if (bucket.empty()) {
                std::cout << "_";
            } else {
                std::cout << bucket.key;
            }
            if (i < buckets.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
    }
};
}

#endif