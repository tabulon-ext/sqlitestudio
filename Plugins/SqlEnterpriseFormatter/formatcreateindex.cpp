#include "formatcreateindex.h"
#include "parser/ast/sqlitecreateindex.h"
#include "parser/ast/sqliteindexedcolumn.h"

FormatCreateIndex::FormatCreateIndex(SqliteCreateIndex* createIndex) :
    createIndex(createIndex)
{
}

void FormatCreateIndex::formatInternal()
{
    handleExplainQuery(createIndex);
    withKeyword("CREATE");
    if (createIndex->uniqueKw)
        withKeyword("UNIQUE");

    withKeyword("INDEX");

    if (createIndex->ifNotExistsKw)
        withKeyword("IF").withKeyword("NOT").withKeyword("EXISTS");

    if (!createIndex->database.isNull())
        withId(createIndex->database).withIdDot();

    withId(createIndex->index).withKeyword("ON").withId(createIndex->table).withParDefLeft().withStatementList(createIndex->indexedColumns).withParDefRight();

    if (createIndex->where)
        withKeyword("WHERE").withStatement(createIndex->where);

    withSemicolon();
}
