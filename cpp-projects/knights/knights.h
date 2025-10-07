#ifndef KNIGHT_H
#define KNIGHT_H

#include <compare>
#include <cstddef>
#include <iostream>
#include <limits>
#include <vector>

class Knight {
private:
  size_t gold;
  size_t weapon;
  size_t armour;

public:
  static constexpr size_t MAX_GOLD = std::numeric_limits<size_t>::max();
  constexpr Knight(size_t g, size_t w, size_t a)
      : gold(g), weapon(w), armour(a) {}

  Knight &operator+=(Knight &);

  Knight(const Knight &) = default;              
  Knight(Knight &&) = default;  
  constexpr Knight &operator=(const Knight &) = default;
  constexpr Knight &operator=(Knight &&) = default;

  constexpr bool operator==(const Knight &x) const {
    // Checking both conditions for equality.
    if ((this->get_weapon_class() > x.get_armour_class() &&
         this->get_armour_class() >= x.get_weapon_class()) ||
        (x.get_weapon_class() > this->get_armour_class() &&
         x.get_armour_class() >= this->get_weapon_class()))
      return false;

    if (x.get_weapon_class() > this->get_armour_class() &&
        this->get_weapon_class() > x.get_armour_class() &&
        (this->get_armour_class() != x.get_armour_class() ||
         x.get_weapon_class() != this->get_weapon_class()))
      return false;

    return true;
  }
  constexpr std::weak_ordering operator<=>(const Knight &x) const {
    if ((*this) == x)
      return std::weak_ordering::equivalent;

    if (this->get_weapon_class() > x.get_armour_class() &&
        this->get_armour_class() >= x.get_weapon_class())
      return std::weak_ordering::greater;

    else if (x.get_weapon_class() > this->get_armour_class() &&
             x.get_armour_class() >= this->get_weapon_class())
      return std::strong_ordering::less;

    // Knights are not equal from previous conditions, value of armour/weapon decides.
    if (auto cmp = this->get_armour_class() <=> x.get_armour_class(); cmp != 0)
      return cmp;

    return this->get_weapon_class() <=> x.get_weapon_class();
  }

  constexpr size_t get_gold() const { return gold; }
  constexpr size_t get_armour_class() const { return armour; }
  constexpr size_t get_weapon_class() const { return weapon; }

  void take_gold(size_t);
  void change_armour(size_t);
  void change_weapon(size_t);
  size_t give_gold();
  size_t take_off_armour();
  size_t give_up_weapon();
  void take_over(Knight &other);
  friend std::ostream &operator<<(std::ostream &os, const Knight &knight);

  constexpr Knight operator+(const Knight &x) const {
    size_t g = x.get_gold();
    size_t My_g = this->get_gold();
    size_t new_g;

    // Preventing overflow.
    if (My_g > Knight::MAX_GOLD - g) {
      new_g = Knight::MAX_GOLD;
    } else {
      new_g = My_g + g;
    }
    return Knight(new_g,
                  (this->get_weapon_class() > x.get_weapon_class())
                      ? this->get_weapon_class()
                      : x.get_weapon_class(),
                  (this->get_armour_class() > x.get_armour_class())
                      ? this->get_armour_class()
                      : x.get_armour_class());
  }
};

static constexpr Knight TRAINEE_KNIGHT(0, 1, 1);

class Tournament {
private:
  std::vector<Knight> pretendents;
  std::vector<Knight> eliminated;

public:
  constexpr Tournament(const std::initializer_list<Knight> &knights) {
    if (knights.size() == 0)
      pretendents.push_back(TRAINEE_KNIGHT);
    else 
      pretendents.insert(pretendents.end(), knights);
    
  }

  const std::vector<Knight> &getPretendents() const { return pretendents; }

  const std::vector<Knight> &getEliminated() const { return eliminated; }

  constexpr size_t size() const {
    return pretendents.size() + eliminated.size();
  }

  Tournament(const Tournament &other);
  constexpr Tournament(const Tournament &&other)
    : pretendents(std::move(other.pretendents)),
      eliminated(std::move(other.eliminated)) {}

  Tournament &operator=(const Tournament &other);

  constexpr Tournament &operator=(const Tournament &&other) {
    if (this != &other) {
      pretendents = std::move(other.pretendents);
      eliminated = std::move(other.eliminated);
    }
    return *this;
  }

  Tournament &operator+=(const Knight &k);

  Tournament &operator-=(const Knight &k);

  const std::vector<Knight>::const_iterator play();
  const std::vector<Knight>::const_iterator no_winner() const {
    return pretendents.end();
  };
  friend std::ostream &operator<<(std::ostream &os, const Tournament &t);
};

constexpr std::pair<size_t, size_t> max_diff_classes(const std::initializer_list<Knight> &knights) {
  size_t current_max_diff = 0;
  std::pair<size_t, size_t> p = {0, 0};

  for (auto k : knights) {
    size_t w = k.get_weapon_class();
    size_t a = k.get_armour_class();

    if (w > a && w - a > current_max_diff)
      current_max_diff = w - a;
    else if (w <= a && a - w > current_max_diff) 
      current_max_diff = a - w;
    
    p.first = w;
    p.second = a;
  }

  return p;
}

#endif // KNIGHT_H