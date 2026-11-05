#pragma once

#include "core/assert.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace jcad::topo
{

  /**
   * Storage for the entities of one kind, addressed by a stable Id.
   *
   * Freed slots are reused by Allocate(). AllocateAt() recreates an entity under a
   * specific Id, which is how undoing a "kill" operator brings back exactly the Ids
   * it removed, so that later undo records stay valid.
   */
  template <typename IdType, typename T>
  class Pool
  {
  public:
    [[nodiscard]] IdType Allocate(T value)
    {
      while (!m_free.empty())
      {
        const IdType id{ m_free.back() };
        m_free.pop_back();
        // AllocateAt() may have revived an Id that is still in the free list.
        if (!IsAlive(id))
        {
          Revive(id, std::move(value));
          return id;
        }
      }
      const IdType id{ static_cast<typename IdType::ValueType>(m_slots.size()) };
      m_slots.push_back(Slot{});
      Revive(id, std::move(value));
      return id;
    }

    void AllocateAt(IdType id, T value)
    {
      JCAD_ASSERT(id.Valid());
      JCAD_ASSERT(!IsAlive(id), "id {} is already alive", id.Value());
      while (m_slots.size() <= id.Value())
      {
        m_free.push_back(IdType{ static_cast<typename IdType::ValueType>(m_slots.size()) });
        m_slots.push_back(Slot{});
      }
      Revive(id, std::move(value));
    }

    void Free(IdType id)
    {
      JCAD_ASSERT(IsAlive(id), "id {} is not alive", id.Value());
      Slot& slot{ m_slots[id.Value()] };
      slot.value = T{};
      slot.alive = false;
      m_free.push_back(id);
      --m_size;
    }

    [[nodiscard]] bool IsAlive(IdType id) const noexcept
    {
      return id.Valid() && id.Value() < m_slots.size() && m_slots[id.Value()].alive;
    }

    [[nodiscard]] const T& Get(IdType id) const
    {
      JCAD_ASSERT(IsAlive(id), "id {} is not alive", id.Value());
      return m_slots[id.Value()].value;
    }

    [[nodiscard]] T& Get(IdType id)
    {
      JCAD_ASSERT(IsAlive(id), "id {} is not alive", id.Value());
      return m_slots[id.Value()].value;
    }

    /** Number of alive entities. */
    [[nodiscard]] std::size_t Size() const noexcept { return m_size; }

    /** Every Id ever handed out is below Capacity(). */
    [[nodiscard]] std::size_t Capacity() const noexcept { return m_slots.size(); }

    /** Alive Ids in increasing order. */
    [[nodiscard]] std::vector<IdType> AliveIds() const
    {
      std::vector<IdType> ids;
      ids.reserve(m_size);
      for (std::size_t i{ 0 }; i < m_slots.size(); ++i)
      {
        if (m_slots[i].alive)
          ids.push_back(IdType{ static_cast<typename IdType::ValueType>(i) });
      }
      return ids;
    }

  private:
    struct Slot
    {
      T value{};
      bool alive{ false };
    };

    void Revive(IdType id, T value)
    {
      Slot& slot{ m_slots[id.Value()] };
      slot.value = std::move(value);
      slot.alive = true;
      ++m_size;
    }

    std::vector<Slot> m_slots;
    std::vector<IdType> m_free;
    std::size_t m_size{ 0 };
  };

} // namespace jcad::topo
