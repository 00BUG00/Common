#pragma once

#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

/**
 * 容器类型特征定义
 * 用于在编译时识别不同类型的容器
 */

// 类型特征：检查是否为序列容器
template<typename T> struct is_sequence_container : std::false_type {};
template<typename T> struct is_sequence_container<std::vector<T>> : std::true_type {};
template<typename T> struct is_sequence_container<std::list<T>> : std::true_type {};
template<typename T> struct is_sequence_container<std::deque<T>> : std::true_type {};

// 类型特征：检查是否为关联容器
template<typename T> struct is_associative_container : std::false_type {};
template<typename K, typename V> struct is_associative_container<std::map<K, V>> : std::true_type {};
template<typename K, typename V> struct is_associative_container<std::unordered_map<K, V>> : std::true_type {};

// 类型特征：检查是否为集合容器
template<typename T> struct is_set_container : std::false_type {};
template<typename T> struct is_set_container<std::set<T>> : std::true_type {};
template<typename T> struct is_set_container<std::unordered_set<T>> : std::true_type {};

// 通用容器类型特征
template<typename T> struct is_container : std::false_type {};
template<typename T> struct is_container<std::vector<T>> : std::true_type {};
template<typename T> struct is_container<std::list<T>> : std::true_type {};
template<typename T> struct is_container<std::deque<T>> : std::true_type {};
template<typename T> struct is_container<std::set<T>> : std::true_type {};
template<typename T> struct is_container<std::unordered_set<T>> : std::true_type {};
template<typename K, typename V> struct is_container<std::map<K, V>> : std::true_type {};
template<typename K, typename V> struct is_container<std::unordered_map<K, V>> : std::true_type {};
