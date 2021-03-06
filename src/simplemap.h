// CSE 375/475 Assignment #1
// Fall 2020
//
// Description: This file specifies a custom map implemented using two vectors.
// << use templates for creating a map of generic types >>
// One vector is used for storing keys. One vector is used for storing values.
// The expectation is that items in equal positions in the two vectors correlate.
// That is, for all values of i, keys[i] is the key for the item in values[i].
// Keys in the map are not necessarily ordered.
//
// The specification for each function appears as comments.
// Students are responsible for implementing the simple map as specified.

#include <vector>
#include <cassert>
#include <functional>
#include <algorithm>

template <class K, class V>
class simplemap_t {

	// Define the two vectors of types K and V
	// << use std::vector<K> >>
	std::vector<K> keys;
	std::vector<V> values;

	private:

		// Custom function to find the index of a specific element in the map
		size_t findIndex(K key) {
			typename std::vector<K>::iterator key_location = std::lower_bound(keys.begin(), keys.end(), key);

			if (*key_location != key)
				return -1;

			return key_location - keys.begin();
		}

	public:

		// The constructor should just initialize the vectors to be empty
		simplemap_t() {
			keys = {};
			values = {};
		}

		// Insert (key, val) if and only if the key is not currently present in
		// the map.  Returns true on success, false if the key was
		// already present.
		bool insert(K key, V val) {
			typename std::vector<K>::iterator key_location = std::lower_bound(keys.begin(), keys.end(), key);

			if (key_location == keys.end())
			{
				values.push_back(val);
				keys.push_back(key);
			}
			else
			{
				if (*key_location == key)
					return false;

				// We must insert into values first because insert into keys will
				// break the index calculation
				values.insert(values.begin() + (key_location - keys.begin()), val);
				keys.insert(key_location, key);
			}

			return true;
		}

		// If key is present in the data structure, replace its value with val
		// and return true; if key is not present in the data structure, return
		// false.
		bool update(K key, V val) {
			size_t index = findIndex(key);

			if (index != -1) {
				values.at(index) = val;

				return true;
			}
			
			return false;
		}

		// Remove the (key, val) pair if it is present in the data structure.
		// Returns true on success, false if the key was not already present.
		bool remove(K key) {
			size_t index = findIndex(key);
			if (index != -1) {
				keys.erase(keys.begin() + index);
				values.erase(values.begin() + index);

				return true;
			}
			
			return false;
		}

		// If key is present in the map, return a pair consisting of
		// the corresponding value and true. Otherwise, return a pair with the
		// boolean entry set to false.
		// Be careful not to share the memory of the map with application threads, you might
		// get unexpected race conditions
		std::pair<V, bool> lookup(K key) {
			size_t index = findIndex(key);

			if (index != -1) {
				return std::make_pair(values.at(index), true);
			}
			return std::make_pair(0, false);
		}

		V sumAll()
		{
			if (values.size() > 0)
			{
				V sum = values.at(0);
				for (int i = 1; i < values.size(); i++)
				{
					sum += values.at(i);
				}

				return sum;
			}
			return 0;
		}

		// Apply a function to each key in the map
		void apply(const std::function<void(K, V)> func) {
			for (size_t i = 0; i < keys.size(); i++) {
				func(keys.at(i), values.at(i));
			}
		}
};
