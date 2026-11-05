#pragma once

#include "euler/euler_op.hpp"

#include <cstddef>
#include <vector>

namespace jcad::topo
{
  class Topology;
}

namespace jcad::euler
{

  /**
   * Applies Euler operators to a topology and records their inverses, grouped into
   * transactions, for undo and redo.
   *
   * Transactions nest: only the outermost Commit() closes an undo entry, and
   * Rollback() undoes what was applied since the matching Begin(). An operator
   * applied outside any transaction is an undo entry of its own. Committing a new
   * entry clears the redo stack.
   */
  class History
  {
  public:
    explicit History(topo::Topology& topology);

    History(const History&) = delete;
    History& operator=(const History&) = delete;
    History(History&&) = delete;
    History& operator=(History&&) = delete;

    [[nodiscard]] const topo::Topology& GetTopology() const noexcept;

    /** Applies `op`, records its inverse and returns it. */
    EulerOp Apply(const EulerOp& op);

    void Begin();
    void Commit();
    void Rollback();
    [[nodiscard]] bool InTransaction() const noexcept;

    [[nodiscard]] std::size_t UndoCount() const noexcept;
    [[nodiscard]] std::size_t RedoCount() const noexcept;

    /** Undoes the last committed entry. No transaction may be open. */
    void Undo();
    void Redo();

  private:
    using Entry = std::vector<EulerOp>;

    /** Applies the entry back to front and returns the inverses in application order,
     *  which is again an entry that this function can replay. */
    Entry Replay(const Entry& entry);

    void Close(Entry entry);

    topo::Topology& m_topology;
    Entry m_open;
    std::vector<std::size_t> m_starts;
    std::vector<Entry> m_undo;
    std::vector<Entry> m_redo;
  };

} // namespace jcad::euler
