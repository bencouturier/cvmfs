/**
 * This file is part of the CernVM File System.
 */

#ifndef CVMFS_STATISTICS_DATABASE_H_
#define CVMFS_STATISTICS_DATABASE_H_

#include <string>
#include "sql.h"

struct RevisionFlags {
  enum T {
    kInitialRevision   = 1,
    kUpdatableRevision = 2,
    kUpdatedRevision   = 3,
    kFailingRevision   = 4,
  };
};

class StatisticsDatabase : public sqlite::Database<StatisticsDatabase> {
 public:
  // not const - needs to be adaptable!
  static float        kLatestSchema;
  // not const - needs to be adaptable!
  static unsigned     kLatestSchemaRevision;
  static const float  kLatestCompatibleSchema;
  static bool         compacting_fails;
  static unsigned int  instances;
  unsigned int         create_empty_db_calls;
  unsigned int         check_compatibility_calls;
  unsigned int         live_upgrade_calls;
  mutable unsigned int compact_calls;

  bool CreateEmptyDatabase();
  bool CheckSchemaCompatibility();
  bool LiveSchemaUpgradeIfNecessary();
  bool CompactDatabase() const;

  ~StatisticsDatabase();

 protected:
  // TODO(rmeusel): C++11 - constructor inheritance
  friend class sqlite::Database<StatisticsDatabase>;
  StatisticsDatabase(const std::string  &filename,
                const OpenMode      open_mode) :
    sqlite::Database<StatisticsDatabase>(filename, open_mode),
    create_empty_db_calls(0),  check_compatibility_calls(0),
    live_upgrade_calls(0), compact_calls(0)
  {
    ++StatisticsDatabase::instances;
  }
};


#endif  // CVMFS_STATISTICS_DATABASE_H_
