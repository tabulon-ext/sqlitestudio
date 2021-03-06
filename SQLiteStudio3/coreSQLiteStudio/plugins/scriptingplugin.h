#ifndef SCRIPTINGPLUGIN_H
#define SCRIPTINGPLUGIN_H

#include "plugin.h"
#include <QVariant>

class Db;

class ScriptingPlugin : virtual public Plugin
{
    public:
        class Context
        {
            public:
                virtual ~Context() {}
        };

        class FunctionInfo
        {
            public:
                virtual QString getName() const = 0;
                virtual QStringList getArguments() const = 0;
                virtual bool getUndefinedArgs() const = 0;
        };

        virtual QString getLanguage() const = 0;
        virtual Context* createContext() = 0;
        virtual void releaseContext(Context* context) = 0;
        virtual void resetContext(Context* context) = 0;
        virtual void setVariable(Context* context, const QString& name, const QVariant& value) = 0;
        virtual QVariant getVariable(Context* context, const QString& name) = 0;
        virtual QVariant evaluate(Context* context, const QString& code, const FunctionInfo& funcInfo,
                                  const QList<QVariant>& args = QList<QVariant>()) = 0;
        virtual bool hasError(Context* context) const = 0;
        virtual QString getErrorMessage(Context* context) const = 0;
        virtual QVariant evaluate(const QString& code, const FunctionInfo& funcInfo, const QList<QVariant>& args = QList<QVariant>(),
                                  QString* errorMessage = nullptr) = 0;
        virtual QString getIconPath() const = 0;
};

class DbAwareScriptingPlugin : public ScriptingPlugin
{
    public:
        virtual QVariant evaluate(Context* context, const QString& code, const FunctionInfo& funcInfo, const QList<QVariant>& args,
                                  Db* db, bool locking = false) = 0;
        virtual QVariant evaluate(const QString& code, const FunctionInfo& funcInfo, const QList<QVariant>& args, Db* db,
                                  bool locking = false, QString* errorMessage = nullptr) = 0;

        QVariant evaluate(Context* context, const QString& code, const FunctionInfo& funcInfo, const QList<QVariant>& args)
        {
            return evaluate(context, code, funcInfo, args, nullptr, true);
        }

        QVariant evaluate(const QString& code, const FunctionInfo& funcInfo, const QList<QVariant>& args, QString* errorMessage = nullptr)
        {
            return evaluate(code, funcInfo, args, nullptr, true, errorMessage);
        }
};

Q_DECLARE_METATYPE(ScriptingPlugin::Context*)

#endif // SCRIPTINGPLUGIN_H
