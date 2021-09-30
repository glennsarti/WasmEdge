// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/runtime/instance/table.h - Table Instance definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the table instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/span.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class TableInstance {
public:
  TableInstance() = delete;
  TableInstance(const RefType &Ref, const AST::Limit &Lim)
      : Type(Ref), HasMaxSize(Lim.hasMax()), MaxSize(Lim.getMax()),
        Refs(Lim.getMin(), UnknownRef()) {}
  virtual ~TableInstance() = default;

  /// Getter of reference type.
  RefType getReferenceType() const noexcept { return Type; }

  /// Get size of table.refs
  uint32_t getSize() const noexcept {
    return static_cast<uint32_t>(Refs.size());
  }

  /// Getter of limit definition.
  bool getHasMax() const noexcept { return HasMaxSize; }

  /// Getter of limit definition.
  uint32_t getMin() const noexcept {
    return static_cast<uint32_t>(Refs.size());
  }

  /// Getter of limit definition.
  uint32_t getMax() const noexcept { return MaxSize; }

  /// Check is out of bound.
  bool checkAccessBound(uint32_t Offset, uint32_t Length) const noexcept {
    const uint64_t AccessLen =
        static_cast<uint64_t>(Offset) + static_cast<uint64_t>(Length);
    return AccessLen <= Refs.size();
  }

  /// Get boundary index.
  uint32_t getBoundIdx() const noexcept {
    return std::max(static_cast<uint32_t>(Refs.size()), UINT32_C(1)) -
           UINT32_C(1);
  }

  /// Grow table with initialization value.
  bool growTable(const uint32_t Count, const RefVariant Val) {
    uint32_t MaxSizeCaped = std::numeric_limits<uint32_t>::max();
    if (HasMaxSize) {
      MaxSizeCaped = std::min(MaxSize, MaxSizeCaped);
    }
    if (Count > MaxSizeCaped - Refs.size()) {
      return false;
    }
    Refs.resize(Refs.size() + Count);
    std::fill_n(Refs.end() - Count, Count, Val);
    return true;
  }
  bool growTable(const uint32_t Count) {
    return growTable(Count, UnknownRef());
  }

  /// Get slice of Refs[Offset : Offset + Length - 1]
  Expect<Span<const RefVariant>> getRefs(const uint32_t Offset,
                                         const uint32_t Length) const noexcept {
    /// Check access boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::TableOutOfBounds);
    }
    return Span<const RefVariant>(Refs.begin() + Offset, Length);
  }

  /// Replace the Refs[Offset :] by Slice[Start : Start + Legnth - 1]
  Expect<void> setRefs(Span<const RefVariant> Slice, const uint32_t Offset,
                       const uint32_t Start, const uint32_t Length) {
    /// Check access boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::TableOutOfBounds);
    }

    /// Check input data validation.
    if (Start + Length > Slice.size()) {
      spdlog::error(ErrCode::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(
          Start, Length,
          std::max(static_cast<uint32_t>(Slice.size()), UINT32_C(1)) -
              UINT32_C(1)));
      return Unexpect(ErrCode::TableOutOfBounds);
    }

    /// Copy references.
    std::copy_n(Slice.begin() + Start, Length, Refs.begin() + Offset);
    return {};
  }

  /// Fill the Refs[Offset : Offset + Length - 1] by Val.
  Expect<void> fillRefs(const RefVariant Val, const uint32_t Offset,
                        const uint32_t Length) {
    /// Check access boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::TableOutOfBounds);
    }

    /// Fill references.
    std::fill_n(Refs.begin() + Offset, Length, Val);
    return {};
  }

  /// Get the elem address.
  Expect<RefVariant> getRefAddr(const uint32_t Idx) const noexcept {
    if (Idx >= Refs.size()) {
      spdlog::error(ErrCode::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getBoundIdx()));
      return Unexpect(ErrCode::TableOutOfBounds);
    }
    return Refs[Idx];
  }

  /// Set the elem address.
  Expect<void> setRefAddr(const uint32_t Idx, const RefVariant Val) {
    if (Idx >= Refs.size()) {
      spdlog::error(ErrCode::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getBoundIdx()));
      return Unexpect(ErrCode::TableOutOfBounds);
    }
    Refs[Idx] = Val;
    return {};
  }

private:
  /// \name Data of table instance.
  /// @{
  const RefType Type;
  const bool HasMaxSize;
  const uint32_t MaxSize;
  std::vector<RefVariant> Refs;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
