#ifndef DATABASESCHEMA_H
#define DATABASESCHEMA_H

/// Database schema
struct DB_column
{
    const char* name; // column name
    const char* type; // datatype of column
};

extern const DB_column databaseSchema[];

#endif // DATABASESCHEMA_H
