#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>

namespace archiveMetrics {
// File-level packed sizes are absent for solid archives. Use the physical
// archive size and the complete listing, never the current folder/selection.
inline double ratio(const QJsonArray& entries, quint64 physical, bool ready) {
  if (!ready || !physical) return -1;
  long double original = 0;
  for (const auto& value : entries) {
    const auto entry = value.toObject();
    if (entry["directory"].toBool()) continue;
    const auto size = entry["size"].toString();
    bool valid = false;
    const auto bytes = size.toULongLong(&valid);
    if (!valid || size.startsWith('-')) return -1;
    original += bytes;
  }
  if (original <= 0) return -1;
  const auto result = static_cast<double>(physical / original);
  return std::isfinite(result) ? result : -1;
}
}
