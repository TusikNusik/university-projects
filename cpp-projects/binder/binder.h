#ifndef BINDER_H
#define BINDER_H

#include <list>
#include <map>
#include <memory>

namespace cxx {
template <typename K, typename V> class binder {

  using list_t = std::list<std::pair<V, const K *>>;
  using list_ptr_t = std::shared_ptr<list_t>;
  using map_t = std::map<K, typename list_t::iterator>;
  using map_ptr_t = std::shared_ptr<map_t>;

public:
  class const_iterator {

  private:
    typename list_t::const_iterator iter;
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = V;
    
    const_iterator() {};

    const_iterator(typename list_t::const_iterator new_iter) : iter{new_iter} {}

    const_iterator(const const_iterator &) = default;

    const_iterator &operator=(const_iterator other) {
      iter = other.iter;
      return *this;
    }

    bool operator==(const_iterator other) const noexcept { return iter == other.iter; }

    const_iterator &operator++() noexcept {
      iter++;
      return *this;
    }

    const_iterator operator++(int) noexcept {
      const_iterator temp = *this;
      iter++;
      return temp;
    }

    const V &operator*() const noexcept { return (*iter).first; }

    const V *operator->() const noexcept { return &((*iter).first); }
  };

  // We make a deep copy of given resources when there is a possibility that user could change binder 'other'.
  binder() noexcept : notes{nullptr}, order_list{nullptr}, has_reference{false} {}
  binder(binder const &other)
      : notes{other.notes}, order_list{other.order_list}, has_reference{other.has_reference} {
    if (has_reference)
      copy_on_write();
  }
  
  binder(binder &&other) noexcept
      : notes{std::move(other.notes)}, order_list{std::move(other.order_list)}, has_reference{std::move(other.has_reference)} {}

  // Exchanges resources using non-throwing swap and realeses the old resource.
  binder &operator=(binder other) {
    swap(other);
    if (has_reference)
      copy_on_write();
    return *this;
  }

  void insert_front(K const &k, V const &v) {
    init();

    // Creates backup variables to guarantee strong exception safety.
    auto notes_temp = notes;
    auto order_list_temp = order_list;
    bool has_reference_temp = has_reference;

    auto it = notes->find(k);
    if (it != notes->end())
      throw std::invalid_argument(
        "Attemped to insert a key that already exists in the binder.");

    copy_on_write(2l);
   
    try {
      order_list->push_front(std::make_pair(v, nullptr));
    } catch (...) {
      notes = notes_temp;
      order_list = order_list_temp;
      has_reference = has_reference_temp;
      throw;
    }

    try {
      (*notes)[k] = order_list->begin();
      order_list->front().second = &notes->find(k)->first;
    } catch (...) {
      order_list->pop_front();
      notes = notes_temp;
      order_list = order_list_temp;
      has_reference = has_reference_temp;
      throw;
    }
  }

  void insert_after(K const &prev_k, K const &k, V const &v) {
    init();
    
    // Creates backup variables to guarantee strong exception safety.
    auto notes_temp = notes;
    auto order_list_temp = order_list;
    bool has_reference_temp = has_reference;

    auto it1 = notes->find(prev_k);
    auto it2 = notes->find(k);
    if(it1 == notes->end()) 
      throw std::invalid_argument(
        "Attemped to insert a key before nonexistent variable.");

    if (it2 != notes->end())
      throw std::invalid_argument(
        "Attemped to insert a key that already exists in the binder.");

    copy_on_write(2l);

    typename list_t::iterator temp;

    try {
      temp = notes->find(prev_k)->second;
      temp++;
      order_list->insert(temp, std::make_pair(v, nullptr));
    } catch (...) {
      notes = notes_temp;
      order_list = order_list_temp;
      has_reference = has_reference_temp;
      throw;
    }

    try {
      temp--;
      (*notes)[k] = temp;
      temp->second = &notes->find(k)->first;
    } catch (...) {
      order_list->erase(temp);
      notes = notes_temp;
      order_list = order_list_temp;
      has_reference = has_reference_temp;
      throw;
    }
  }

  void remove() {
    throw_if_empty();

    copy_on_write();

    auto first_elem = order_list->front();
    notes->erase(*(first_elem.second));
    order_list->pop_front();
  }

  void remove(K const &k) {
    throw_if_empty();

    auto it = notes->find(k);
    if (it == notes->end())
      throw std::invalid_argument(
        "Attemped to remove nonexistent note.");

    copy_on_write();

    auto it2 = (*notes)[k];
    order_list->erase(it2);
    notes->erase(k);
  }

  V &read(K const &k) {
    throw_if_empty();

    auto it = notes->find(k);
    if (it == notes->end())
      throw std::invalid_argument(
        "Attemped to read a from a note with a key that doesn't exist in the binder.");

    // This binder has an active reference.
    if (!copy_on_write())
      has_reference = true;

    return notes->find(k)->second->first;
  }

  V const &read(K const &k) const {
    throw_if_empty();

    auto it = notes->find(k);
    if (it == notes->end())
      throw std::invalid_argument(
        "Attemped to read a from a note with a key that doesn't exist in the binder.");

    return it->second->first;
  }

  std::size_t size() const noexcept {
    if (notes == nullptr || order_list == nullptr)
      return 0;

    return notes->size();
  }

  void clear() noexcept {
    notes = nullptr;
    order_list = nullptr;
    has_reference = false;
  }

  const_iterator cbegin() const noexcept {
    if (order_list == nullptr)
      return const_iterator{};

    return const_iterator{order_list->cbegin()};
  }

  const_iterator cend() const noexcept {
    if (order_list == nullptr)
      return const_iterator{};

    return const_iterator{order_list->cend()};
  }

private:
  // Bool has_reference is true whenever binder didn't make a deep copy during non-const 'read' function.
  map_ptr_t notes;
  list_ptr_t order_list;
  bool has_reference = false;

  void init() {
    if (notes == nullptr)
      notes = std::make_shared<map_t>();

    if (order_list == nullptr)
      order_list = std::make_shared<list_t>();
  }

  void throw_if_empty() const {
    if (size() == 0)
      throw std::invalid_argument("This operation can't be performed on an empty binder.");
  }

  // Returns true iff copy-on-write actually happens.
  bool copy_on_write(long expected_count = 1) {
		if (notes == nullptr || order_list == nullptr)
      return false;
    if (notes.use_count() <= expected_count && order_list.use_count() <= expected_count && !has_reference)
      return false;

    auto notes_temp = notes;
    auto order_list_temp = order_list;
    bool has_reference_temp = has_reference;

    // Makes a deep copy of given resources.
    try {
      notes = std::make_shared<map_t>();
      order_list = std::make_shared<list_t>();
      has_reference = false;

      for (auto it = order_list_temp->cend(); it != order_list_temp->cbegin();) {
        it--;
        V v = it->first;
        K k = *it->second;
        order_list->push_front(std::make_pair(v, nullptr));
        (*notes)[k] = order_list->begin();
        order_list->front().second = &notes->find(k)->first;
      }
    } catch (...) {
      notes = notes_temp;
      order_list = order_list_temp;
      has_reference = has_reference_temp;
      throw;
    }
    return true;
  }

  void swap(binder &other) noexcept {
    std::swap(this->notes, other.notes);
    std::swap(this->order_list, other.order_list);
    has_reference = other.has_reference;
  }
};
} // namespace cxx
#endif