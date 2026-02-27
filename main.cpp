#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "cvitem.h" // 👈 1. 引入咱们刚刚辛辛苦苦写的 C++ 头文件

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // 👈 2. 把我们的 C++ 类注册为 QML 可以认识的组件！
    // 意思是：我们给它起了个包名叫 "MyOpenCV"，版本 1.0，控件名叫 "CvItem"
    qmlRegisterType<CvItem>("MyOpenCV", 1, 0, "CvItem");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // 注意：这里的 opcvtest 是你项目的默认模块名，保持不变
    engine.loadFromModule("opcvtest", "Main");

    return app.exec();
}
