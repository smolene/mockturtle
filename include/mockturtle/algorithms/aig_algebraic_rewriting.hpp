/*!
  \file aig_algebraic_rewriting.hpp
  \brief AIG algebraric rewriting

  EPFL CS-472 2021 Final Project Option 1
*/

#pragma once

#include "../networks/aig.hpp"
#include "../views/depth_view.hpp"
#include "../views/topo_view.hpp"
#include <mockturtle/io/write_dot.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace mockturtle {

namespace detail {

//using Ntk = depth_view<aig_network>;

template <class Ntk> class aig_algebraic_rewriting_impl {
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  aig_algebraic_rewriting_impl(Ntk& ntk) : ntk(ntk) {
    static_assert(has_level_v<Ntk>, "Ntk does not implement depth interface.");
  }

  void run() {
    bool cont{true}; /* continue trying */
    int counter = 0;
    while (cont) {
      std::string filename = "dot/loop_";
      filename += std::to_string(counter);
      filename += ".dot";
      write_dot(ntk, filename);
      counter++;

      cont = false; /* break the loop if no updates can be made */
      ntk.foreach_gate([&](node n) {
        if (try_algebraic_rules(n)) {
          ntk.update_levels();
          cont = true;
        }
      });
    }
  }

private:
  /* Try various algebraic rules on node n. Return true if the network is updated. */
  bool try_algebraic_rules(node n) {
    if (try_associativity(n))
      return true;
    if (try_distributivity(n))
      return true;
    /* TODO: add more rules here... */

    return false;
  }

  std::vector<signal> push_fanins(node n) {
    std::vector<signal> v;
    ntk.foreach_fanin(n, [&](signal const& s) {
      v.push_back(s);
    });
    return v;
  }

  std::tuple<int, signal, signal> critical_fanins(node n) {
    signal a, b;
    int count = 0;
    ntk.foreach_fanin(n, [&](signal const& s) {
      if (count == 0) {
        a = s;
      } else {
        b = s;
      }
      count += 1;
    });

    if (count != 2) {
      throw std::runtime_error("Not 2 inputs to node.");
    }

    node na = ntk.get_node(a);
    node nb = ntk.get_node(b);
    bool ba = ntk.is_on_critical_path(na);
    bool bb = ntk.is_on_critical_path(nb);
    if (ba) {
      if (bb) {
        return std::tuple{2, a, b};
      } else {
        return std::tuple{1, a, b};
      }
    } else {
      if (bb) {
        return std::tuple{1, b, a};
      } else {
        throw std::runtime_error("0 input are critical paths.");
      }
    }
  }

  /* Try the associativity rule on node n. Return true if the network is updated. */
  bool try_associativity(node root) {
    if (!ntk.is_on_critical_path(root) || ntk.is_pi(root)) {
      return false;
    }

    auto [count_root, crit_root_mid, noncrit_root_mid] = critical_fanins(root);

    // nothing to do, both paths are maxed out already.
    if (count_root == 2) {
      return false;
    }

    auto const mid = ntk.get_node(crit_root_mid);
    auto const noncrit_mid = ntk.get_node(noncrit_root_mid);

    if (ntk.is_pi(mid) || ntk.is_complemented(crit_root_mid)) {
      return false;
    }

    auto [count_mid, crit_mid_top, noncrit_mid_top] = critical_fanins(mid);

    // nothing to do, both paths are maxed out already. Maybe useless, risky though.
    if (count_mid == 2) {
      return false;
    }

    auto const top = ntk.get_node(crit_mid_top);
    auto const noncrit_top = ntk.get_node(noncrit_mid_top);

    auto const noncrit_and = ntk.create_and(noncrit_root_mid, noncrit_mid_top);
    auto const crit_and = ntk.create_and(noncrit_and, crit_mid_top);

    ntk.substitute_node(root, crit_and);

    return true;
  }

  /* Try the distributivity rule on node n. Return true if the network is updated. */
  bool try_distributivity(node n) {
    /* TODO */
    return false;
  }

private:
  Ntk& ntk;
  // TODO
};

} // namespace detail

/* Entry point for users to call */
template <class Ntk> void aig_algebraic_rewriting(Ntk& ntk) {
  static_assert(std::is_same_v<typename Ntk::base_type, aig_network>, "Ntk is not an AIG");

  depth_view dntk{ntk};
  detail::aig_algebraic_rewriting_impl p(dntk);
  p.run();
}

} /* namespace mockturtle */