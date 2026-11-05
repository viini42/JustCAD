#include "euler/history.hpp"

#include "core/assert.hpp"
#include "topo/topology.hpp"

#include <utility>

jcad::euler::History::History(topo::Topology& topology) : m_topology{ topology } {}

const jcad::topo::Topology& jcad::euler::History::GetTopology() const noexcept
{
  return m_topology;
}

jcad::euler::EulerOp jcad::euler::History::Apply(const EulerOp& op)
{
  EulerOp inverse{ euler::Apply(m_topology, op) };
  if (InTransaction())
    m_open.push_back(inverse);
  else
    Close(Entry{ inverse });
  return inverse;
}

void jcad::euler::History::Begin()
{
  m_starts.push_back(m_open.size());
}

void jcad::euler::History::Commit()
{
  JCAD_ASSERT(InTransaction(), "Commit() without Begin()");
  m_starts.pop_back();
  if (InTransaction() || m_open.empty())
    return;
  Close(std::exchange(m_open, Entry{}));
}

void jcad::euler::History::Rollback()
{
  JCAD_ASSERT(InTransaction(), "Rollback() without Begin()");
  const std::size_t start{ m_starts.back() };
  m_starts.pop_back();
  const Entry tail(m_open.begin() + static_cast<std::ptrdiff_t>(start), m_open.end());
  m_open.resize(start);
  static_cast<void>(Replay(tail));
}

bool jcad::euler::History::InTransaction() const noexcept
{
  return !m_starts.empty();
}

std::size_t jcad::euler::History::UndoCount() const noexcept
{
  return m_undo.size();
}

std::size_t jcad::euler::History::RedoCount() const noexcept
{
  return m_redo.size();
}

void jcad::euler::History::Undo()
{
  JCAD_ASSERT(!InTransaction(), "cannot undo inside a transaction");
  JCAD_ASSERT(!m_undo.empty(), "nothing to undo");
  Entry entry{ std::move(m_undo.back()) };
  m_undo.pop_back();
  m_redo.push_back(Replay(entry));
}

void jcad::euler::History::Redo()
{
  JCAD_ASSERT(!InTransaction(), "cannot redo inside a transaction");
  JCAD_ASSERT(!m_redo.empty(), "nothing to redo");
  Entry entry{ std::move(m_redo.back()) };
  m_redo.pop_back();
  m_undo.push_back(Replay(entry));
}

jcad::euler::History::Entry jcad::euler::History::Replay(const Entry& entry)
{
  Entry inverses;
  inverses.reserve(entry.size());
  for (auto op{ entry.rbegin() }; op != entry.rend(); ++op)
    inverses.push_back(euler::Apply(m_topology, *op));
  return inverses;
}

void jcad::euler::History::Close(Entry entry)
{
  m_undo.push_back(std::move(entry));
  m_redo.clear();
}
