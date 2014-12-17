#ifndef QUERY_H
#define QUERY_H
#include <deque>
#include <string>

// every column is an element of cRow
typedef std::deque<std::string> cRow;
typedef std::deque<std::string> cFields;
typedef std::deque<std::string> cWhereClauses;

class cQuery
{
private:
     cFields fields; // columns to be "selected"
     cWhereClauses whereClauses; // filters to be applies on result
     std::string optionClause; // ORDER BY etc
public:
     std::string table;
    void AddField(const std::string& field);
    void AddWhereClause(const std::string& clause);
    void SetOptionClause(const std::string& opt);

    std::deque<std::string> Fields() const;
    std::deque<std::string> WhereClauses() const;
    std::string OptionClause() const;

};

struct cResults
{
    // 'Fields' are column names like "title", "artist", "year" etc
    cFields fields;

    std::deque<cRow> results;

    std::string errorMesg;
    int sql_errorno;
};

#endif // QUERY_H
