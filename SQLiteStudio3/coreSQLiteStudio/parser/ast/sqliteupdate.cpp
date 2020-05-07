#include "sqliteupdate.h"
#include "sqlitequerytype.h"
#include "sqliteexpr.h"
#include "parser/statementtokenbuilder.h"
#include "common/global.h"
#include "sqlitewith.h"
#include <QDebug>

SqliteUpdate::SqliteUpdate()
{
    queryType = SqliteQueryType::Update;
}

SqliteUpdate::SqliteUpdate(const SqliteUpdate& other) :
    SqliteQuery(other), onConflict(other.onConflict), database(other.database), table(other.table), indexedByKw(other.indexedByKw),
    notIndexedKw(other.notIndexedKw), indexedBy(other.indexedBy)
{
    // Special case of deep collection copy
    SqliteExpr* newExpr = nullptr;
    for (const ColumnAndValue& keyValue : other.keyValueMap)
    {
        newExpr = new SqliteExpr(*keyValue.second);
        newExpr->setParent(this);
        keyValueMap << ColumnAndValue(keyValue.first, newExpr);
    }

    DEEP_COPY_FIELD(SqliteExpr, where);
    DEEP_COPY_FIELD(SqliteWith, with);
}

SqliteUpdate::~SqliteUpdate()
{
}

SqliteUpdate::SqliteUpdate(SqliteConflictAlgo onConflict, const QString &name1, const QString &name2, bool notIndexedKw, const QString &indexedBy,
                           const QList<ColumnAndValue>& values, SqliteExpr *where, SqliteWith* with)
    : SqliteUpdate()
{
    this->onConflict = onConflict;

    if (!name2.isNull())
    {
        database = name1;
        table = name2;
    }
    else
        table = name1;

    this->indexedBy = indexedBy;
    this->indexedByKw = !(indexedBy.isNull());
    this->notIndexedKw = notIndexedKw;
    keyValueMap = values;

    this->where = where;
    if (where)
        where->setParent(this);

    this->with = with;
    if (with)
        with->setParent(this);

    for (const ColumnAndValue& keyValue : keyValueMap)
        keyValue.second->setParent(this);
}

SqliteStatement*SqliteUpdate::clone()
{
    return new SqliteUpdate(*this);
}

SqliteExpr* SqliteUpdate::getValueForColumnSet(const QString& column)
{
    for (const ColumnAndValue& keyValue : keyValueMap)
    {
        if (keyValue.first == column)
            return keyValue.second;
    }
    return nullptr;
}

QStringList SqliteUpdate::getColumnsInStatement()
{
    QStringList columns;
    for (const ColumnAndValue& keyValue : keyValueMap)
    {
        if (keyValue.first.type() == QVariant::StringList)
            columns += keyValue.first.toStringList();
        else
            columns += keyValue.first.toString();
    }

    return columns;
}

QStringList SqliteUpdate::getTablesInStatement()
{
    return getStrListFromValue(table);
}

QStringList SqliteUpdate::getDatabasesInStatement()
{
    return getStrListFromValue(database);
}

TokenList SqliteUpdate::getColumnTokensInStatement()
{
    // This case is not simple. We only have "setlist" in tokensMap
    // and it contains entire: col = expr, col = expr.
    // In order to extract 'col' token, we go through all 'expr',
    // for each 'expr' we get its first token, then locate it
    // in entire "setlist", get back 2 tokens to get what's before "=".
    TokenList list;
    TokenList setListTokens = getTokenListFromNamedKey("setlist", -1);
    int setListTokensSize = setListTokens.size();
    int end;
    int start = 0;
    SqliteExpr* expr = nullptr;
    for (const ColumnAndValue& keyValue : keyValueMap)
    {
        expr = keyValue.second;
        end = setListTokens.indexOf(expr->tokens[0]);
        if (end < 0 || end >= setListTokensSize)
        {
            qCritical() << "Went out of bounds while looking for column tokens in SqliteUpdate::getColumnTokensInStatement().";
            continue;
        }

        // Before expression tokens there will be only column(s) token(s)
        // and commans, and equal operator. Let's take only ID tokens, which are columns.
        list += setListTokens.mid(start, end - start - 1).filter(Token::OTHER);

        start = end + expr->tokens.size();
    }
    return list;
}

TokenList SqliteUpdate::getTableTokensInStatement()
{
    if (tokensMap.contains("fullname"))
        return getObjectTokenListFromFullname();

    return TokenList();
}

TokenList SqliteUpdate::getDatabaseTokensInStatement()
{
    if (tokensMap.contains("fullname"))
        return getDbTokenListFromFullname();

    if (tokensMap.contains("nm"))
        return extractPrintableTokens(tokensMap["nm"]);

    return TokenList();
}

QList<SqliteStatement::FullObject> SqliteUpdate::getFullObjectsInStatement()
{
    QList<FullObject> result;
    if (!tokensMap.contains("fullname"))
        return result;

    // Table object
    FullObject fullObj = getFullObjectFromFullname(FullObject::TABLE);

    if (fullObj.isValid())
        result << fullObj;

    // Db object
    fullObj = getFirstDbFullObject();
    if (fullObj.isValid())
    {
        result << fullObj;
        dbTokenForFullObjects = fullObj.database;
    }

    return result;
}

TokenList SqliteUpdate::rebuildTokensFromContents()
{
    StatementTokenBuilder builder;
    builder.withTokens(SqliteQuery::rebuildTokensFromContents());
    if (with)
        builder.withStatement(with);

    builder.withKeyword("UPDATE").withSpace();
    if (onConflict != SqliteConflictAlgo::null)
        builder.withKeyword("OR").withSpace().withKeyword(sqliteConflictAlgo(onConflict)).withSpace();

    if (!database.isNull())
        builder.withOther(database).withOperator(".");

    builder.withOther(table).withSpace();

    if (indexedByKw)
        builder.withKeyword("INDEXED").withSpace().withKeyword("BY").withSpace().withOther(indexedBy).withSpace();
    else if (notIndexedKw)
        builder.withKeyword("NOT").withSpace().withKeyword("INDEXED").withSpace();

    builder.withKeyword("SET").withSpace();

    bool first = true;
    for (const ColumnAndValue& keyVal : keyValueMap)
    {
        if (!first)
            builder.withOperator(",").withSpace();

        if (keyVal.first.type() == QVariant::StringList)
            builder.withParLeft().withOtherList(keyVal.first.toStringList()).withParRight();
        else
            builder.withOther(keyVal.first.toString());

        builder.withSpace().withOperator("=").withStatement(keyVal.second);
        first = false;
    }

    if (where)
        builder.withSpace().withKeyword("WHERE").withStatement(where);

    builder.withOperator(";");

    return builder.build();
}
