#include "query.h"

////////////////////////////////////////////////////////////////
/// cQuery
////////////////////////////////////////////////////////////////
void cQuery::AddField(const std::string &field)
{
    if (!field.empty())
        fields.push_back(field);
}

void cQuery::AddWhereClause(const std::string &clause)
{
    if(!clause.empty())
        whereClauses.push_back(clause);
}

void cQuery::SetOptionClause(const std::string &opt)
{
    if (!opt.empty())
        optionClause = opt;
}

std::string cQuery::OptionClause() const
{
    return optionClause;
}

std::vector<std::string> cQuery::Fields() const
{
    return fields;
}

std::vector<std::string> cQuery::WhereClauses() const
{
    return whereClauses;
}


