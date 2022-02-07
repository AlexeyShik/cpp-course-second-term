#pragma once
#include "function.h"
#include "intrusive_list.h"

// Чтобы не было коллизий с UNIX-сигналами реализация вынесена в неймспейс, по
// той же причине изменено и название файла
namespace signals {

template <typename T>
struct signal;

template<typename... Args>
struct signal<void(Args...)> {
  using slot_t = function<void(Args...)>;

  struct connection : intrusive::list_element<struct connection_tag> {

    connection() noexcept = default;

    connection(connection &&other) noexcept : slot(other.slot), sig(other.sig) {
      connection_move(std::move(other));
    }

    connection &operator=(connection &&other) noexcept {
      if (this != &other) {
        slot = other.slot;
        sig = other.sig;
        connection_move(std::move(other));
      }
      return *this;
    }

    void disconnect() noexcept {
      if (sig) {
        for (iteration_token* tok = sig->top_token; tok != nullptr; tok = tok->next) {
          if (&*tok->it == this) {
            ++tok->it;
          }
        }

        unlink();
        slot = slot_t();
        sig = nullptr;
      }
    }

    ~connection() noexcept {
      disconnect();
    }

    friend signal<void(Args...)>;
  private:
    signal* sig{nullptr};
    slot_t slot{};

    connection(const slot_t &slot, signal* sig) noexcept : slot(slot), sig(sig) {}

    void connection_move(connection &&other) noexcept {
      if (!other.sig && !sig) {
        return;
      }
      for (iteration_token* tok = sig->top_token; tok != nullptr; tok = tok->next) {
        if (&*tok->it == &other) {
          tok->it = intrusive::list<connection, connection_tag>::to_list_iterator(*this);
        }
      }
      static_cast<list_element<connection_tag>&>(*this) =
          std::move(static_cast<list_element<connection_tag>&>(other));
    }
  };

  signal() noexcept = default;

  signal(const signal &) = delete;

  signal operator=(const signal &) = delete;

  ~signal() noexcept {
    for (iteration_token* tok = top_token; tok != nullptr; tok = tok->next) {
      tok->sig = nullptr;
    }

    while (!cons.empty()) {
      auto it = cons.begin();
      it->sig = nullptr;
      it->unlink();
    }
  };

  connection connect(slot_t slot) noexcept {
    connection curr(slot, this);
    cons.push_back(curr);
    return curr;
  }

  using connections_t = intrusive::list<connection, connection_tag>;

  void operator()(Args... args) const {
    iteration_token tok(this);

    while (tok.it != cons.end()) {
      typename connections_t::const_iterator cur = tok.it;
      ++tok.it;

      cur->slot(args...);

      if (tok.sig == nullptr) {
        break;
      }
    }
  }

private:
  struct iteration_token {

    explicit iteration_token(const signal* sig) noexcept : sig(sig),
          it(sig->cons.begin()), next(sig->top_token) {
      sig->top_token = this;
    }

    ~iteration_token() noexcept {
      if (sig) {
        sig->top_token = next;
      }
    }

    const signal* sig; // nullptr = sig not exists
    typename connections_t::const_iterator it;
    iteration_token* next;
  };

  connections_t cons;
  mutable iteration_token* top_token = nullptr;
};

} // namespace signals