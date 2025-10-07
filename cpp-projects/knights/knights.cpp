#include "knights.h"

void Knight::take_gold(size_t x) {
  // Preventing overflow.
  if (x > MAX_GOLD - gold)
    gold = MAX_GOLD;
  else
    gold += x;
}

void Knight::change_armour(size_t x) { armour = x; }

void Knight::change_weapon(size_t x) { weapon = x; }

size_t Knight::give_gold() {
  size_t x = gold;
  gold = 0;
  return x;
}

size_t Knight::take_off_armour() {
  size_t x = armour;
  armour = 0;
  return x;
}

size_t Knight::give_up_weapon() {
  size_t x = weapon;
  weapon = 0;
  return x;
}

void Knight::take_over(Knight &other) {
  this->take_gold(other.give_gold());

  size_t ar = other.get_armour_class();
  if (ar > this->get_armour_class()) {
    this->change_armour(ar);
    other.change_armour(0);
  }

  size_t w = other.get_weapon_class();

  if (w > this->get_weapon_class()) {
    this->change_weapon(w);
    other.change_weapon(0);
  }
}

std::ostream &operator<<(std::ostream &os, const Knight &knight) {
  os << "(" << knight.get_gold() << " gold, " << knight.get_weapon_class()
     << " weapon class, " << knight.get_armour_class() << " armour class)\n";
  return os;
}

Knight &Knight::operator+=(Knight &x) {
  // Preventing '*this' from using '+=' on itself.
  if (this == &x)
    return *this;

  size_t g = x.give_gold();
  size_t My_g = this->give_gold();

  // Preventing overflow.
  if (My_g > Knight::MAX_GOLD - g) {
    this->take_gold(Knight::MAX_GOLD);
  } else {
    this->take_gold(My_g + g);
  }

  if (this->get_armour_class() < x.get_armour_class())
    this->change_armour(x.take_off_armour());

  if (this->get_weapon_class() < x.get_weapon_class())
    this->change_weapon(x.give_up_weapon());

  return *this;
}

std::ostream &operator<<(std::ostream &os, const Tournament &t) {
  for (auto &knight : t.getPretendents()) {
    os << "+ " << knight;
  }

  for (auto &knight : t.getEliminated()) {
    os << "- " << knight;
  }

  os << "=\n";
  return os;
}

Tournament::Tournament(const Tournament &other)
    : pretendents(other.pretendents), eliminated(other.eliminated) {}

Tournament &Tournament::operator=(const Tournament &other) {
  if (this != &other) {
    pretendents = other.pretendents;
    eliminated = other.eliminated;
  }

  return *this;
}

Tournament &Tournament::operator+=(const Knight &k) {
  pretendents.push_back(Knight(k));
  eliminated.clear();
  return *this;
}

Tournament &Tournament::operator-=(const Knight &k) {
  size_t g = k.get_gold();
  size_t a = k.get_armour_class();
  size_t w = k.get_weapon_class();
  for (auto it = pretendents.begin(); it != pretendents.end();) {
    if (it->get_gold() == g && it->get_armour_class() == a &&
        it->get_weapon_class() == w) {
        it = pretendents.erase(it); // Update iterator after erase
    } else {
        ++it; // Only increment if not erased
    }
  }

  eliminated.clear();
  return *this;
}

const std::vector<Knight>::const_iterator Tournament::play() {
  eliminated.clear();
  while (pretendents.size() > 1) {
    Knight knight1 = pretendents.front();
    pretendents.erase(pretendents.begin());

    Knight knight2 = pretendents.front();
    pretendents.erase(pretendents.begin());
    
    if (knight1 > knight2) {
      knight1.take_over(knight2);
      pretendents.push_back(knight1);
      eliminated.insert(eliminated.begin(), knight2);
    } else if (knight1 < knight2) {
      knight2.take_over(knight1);
      pretendents.push_back(knight2);
      eliminated.insert(eliminated.begin(), knight1);
    } else {
      eliminated.insert(eliminated.begin(), knight2);
      eliminated.insert(eliminated.begin(), knight1);
    }
  }

  if (pretendents.size() == 1) {
    return pretendents.cbegin();
  } else {
    return pretendents.cend();
  }
}