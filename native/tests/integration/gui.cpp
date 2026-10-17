#include "../../app/AppController.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QTest>
#include <QSignalSpy>
#include <QFile>
#include <QDir>
#include <QElapsedTimer>
#include <functional>
#include <iostream>
class GuiFlow : public QObject {
  Q_OBJECT
 public:QString worker,library,archive,qml;QTemporaryDir work;
 private slots:
  void passwordAndSelectedConflicts(){
    AppController api(worker,library,nullptr,work.path()+"/data");api.setPreference("language","en");
    QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("backend",&api);engine.rootContext()->setContextProperty("designPreview",false);engine.load(QUrl::fromLocalFile(qml));
    QVERIFY(!engine.rootObjects().isEmpty());auto root=engine.rootObjects().first();auto window=qobject_cast<QQuickWindow*>(root);QVERIFY(window);
    auto wait=[&](std::function<bool()> predicate){QElapsedTimer timer;timer.start();while(!predicate()&&timer.elapsed()<12000)QTest::qWait(10);return predicate();};
    std::function<QQuickItem*(QQuickItem*,QString)> visual;
    visual=[&](QQuickItem* parent,QString name)->QQuickItem*{if(parent->objectName()==name)return parent;for(auto child:parent->childItems())if(auto found=visual(child,name))return found;return nullptr;};
    auto object=[&](const char* name)->QObject*{if(auto found=root->findChild<QObject*>(name))return found;return visual(window->contentItem(),name);};
    auto opened=[&](const char* name){auto p=object(name);return p&&p->property("opened").toBool();};
    auto click=[&](const char* name){auto item=qobject_cast<QQuickItem*>(object(name));if(!item||!item->isVisible()||!item->isEnabled())return false;QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,item->mapToScene(item->boundingRect().center()).toPoint());return true;};
    auto password=[&](QByteArray value){if(!wait([&]{return opened("passwordDialog")&&api.jobState()=="AwaitingPassword";}))return false;auto field=qobject_cast<QQuickItem*>(object("passwordField"));if(!field)return false;field->forceActiveFocus();for(char c:value)QTest::keyClick(window,c);QTest::keyClick(window,Qt::Key_Return);return true;};
#ifdef Q_OS_WIN
    {QFile origin(archive+":Zone.Identifier");QVERIFY(origin.open(QIODevice::WriteOnly));origin.write("[ZoneTransfer]\r\nZoneId=3\r\n");}
#endif
    api.openUrls({QUrl::fromLocalFile(archive)});QVERIFY(password("incorrect"));QVERIFY(wait([&]{return opened("passwordDialog")&&api.jobState()=="AwaitingPassword"&&object("passwordField")->property("text").toString().isEmpty();}));
    QVERIFY(password("test-only"));QVERIFY(wait([&]{return api.archiveFormat()=="7z"&&!api.busy();}));
    QVERIFY(!window->grabWindow().isNull());auto files=qobject_cast<QQuickItem*>(object("filesList"));QVERIFY(files);QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,files->mapToScene(QPointF(100,20)).toPoint());QVERIFY(api.hasSelection());
    const auto output=work.path()+"/extract";QDir().mkpath(output);const auto target=output+'/'+QString::fromUtf8("中文 😀.txt");{QFile file(target);QVERIFY(file.open(QIODevice::WriteOnly));file.write("keep");}
    auto startExtract=[&]{if(!click("extractButton")||!wait([&]{return opened("extractDialog");}))return false;object("extractDestination")->setProperty("text",output);object("smartExtract")->setProperty("checked",false);object("openAfter")->setProperty("checked",false);return click("extractStart");};
    QSignalSpy results(&api,&AppController::operationCompleted);QVERIFY(startExtract());QVERIFY(password("test-only"));QVERIFY(wait([&]{return opened("conflictDialog");}));QVERIFY(click("conflict-skip"));QVERIFY(wait([&]{return !api.busy();}));
    {QFile file(target);QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("keep"));}
    QVERIFY(startExtract());QVERIFY(password("test-only"));QVERIFY(wait([&]{return opened("conflictDialog");}));QVERIFY(click("conflict-rename"));QVERIFY(wait([&]{return !api.busy();}));QCOMPARE(QDir(output).entryList(QDir::Files).size(),2);
    {QFile file(target);QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("keep"));}
#ifdef Q_OS_WIN
    {const auto files=QDir(output).entryList(QDir::Files);const auto other=files[0]==QFileInfo(target).fileName()?files[1]:files[0];QFile origin(output+'/'+other+":Zone.Identifier");QVERIFY(origin.open(QIODevice::ReadOnly));QVERIFY(origin.readAll().contains("ZoneId=3"));}
#endif
    // Escape is a real GUI cancel path, followed by a fresh operation in the same window.
    QVERIFY(startExtract());QVERIFY(wait([&]{return opened("passwordDialog");}));QTest::keyClick(window,Qt::Key_Escape);QVERIFY(wait([&]{return !api.busy();}));api.testArchive();QVERIFY(password("test-only"));QVERIFY(wait([&]{return !api.busy();}));
    bool tested=false;for(const auto& result:results)if(result[0].toString()=="test")tested=true;QVERIFY(tested);
  }
  void destructionDuringPasswordChallenge(){
    auto api=std::make_unique<AppController>(worker,library,nullptr,work.path()+"/close-data");bool challenged=false;
    connect(api.get(),&AppController::challenge,this,[&]{challenged=true;});api->openUrls({QUrl::fromLocalFile(archive)});
    QTRY_VERIFY_WITH_TIMEOUT(challenged,12000);QElapsedTimer timer;timer.start();api.reset();QVERIFY(timer.elapsed()<5000);
  }
};
int main(int argc,char** argv){QGuiApplication app(argc,argv);QQuickStyle::setStyle("Basic");if(app.arguments().size()!=5)return 2;GuiFlow flow;flow.worker=app.arguments()[1];flow.library=app.arguments()[2];flow.archive=app.arguments()[3];flow.qml=app.arguments()[4];auto file=QDir::current().filePath("native-gui-flow.txt");auto output=(file+",txt").toUtf8();char* options[]={argv[0],const_cast<char*>("-o"),output.data()};const auto result=QTest::qExec(&flow,3,options);QFile log(file);if(log.open(QIODevice::ReadOnly))std::cout<<log.readAll().toStdString()<<std::flush;return result;}
#include "gui.moc"
