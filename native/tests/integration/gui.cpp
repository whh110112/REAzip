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
#include <QJsonArray>
#include <QJsonDocument>
#include <QFontDatabase>
#include <QQmlComponent>
#include <QImage>
#include <limits>
#include <functional>
#include <iostream>
class GuiFlow : public QObject {
  Q_OBJECT
 public:QString worker,library,archive,qml;QTemporaryDir work;
 private slots:
  void adaptiveLogoRendering();
  void archiveTabLayoutAndInteraction(){
    AppController api(worker,library,nullptr,work.path()+"/tab-layout");
    QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("backend",&api);engine.rootContext()->setContextProperty("designPreview",false);engine.load(QUrl::fromLocalFile(qml));
    QVERIFY(!engine.rootObjects().isEmpty());auto root=engine.rootObjects().first();auto window=qobject_cast<QQuickWindow*>(root);QVERIFY(window);
    window->resize(880,560);QSignalSpy warnings(&engine,&QQmlEngine::warnings);
    const auto fixture=QFileInfo(qml).absoluteDir().filePath("../tests/fixtures/sample.iso");
    const auto sample=work.path()+"/sample.iso";QVERIFY(QFile::copy(fixture,sample));
    api.openUrls({QUrl::fromLocalFile(sample)});QTRY_VERIFY_WITH_TIMEOUT(!api.busy()&&api.archiveFormat()=="iso",12000);
    const auto firstId=api.activeTab();
    const QHash<QString,QString> names={
      {"en","Archive with a long filename"},{"zh-CN","中文归档文件"},{"zh-TW","繁體中文歸檔"},
      {"ja","日本語のアーカイブ"},{"ko","한국어 압축 파일"},{"de","Ausführlicher Archivname"},
      {"fr","Français — été"},{"es","Español — compresión"},{"pt-BR","Português — extração"},
      {"it","Archivio italiano"},{"ru","Русский архив"},{"pl","Polskie archiwum"},
      {"tr","Türkçe arşiv"},{"id","Arsip Indonesia"},{"vi","Tệp lưu trữ tiếng Việt"},
      {"ar","ملف أرشيف عربي"},{"hi","हिन्दी संग्रह फ़ाइल"},{"th","ไฟล์บีบอัดภาษาไทย"}
    };
    std::function<QQuickItem*(QQuickItem*,QString)> find;
    find=[&](QQuickItem* item,QString name)->QQuickItem*{if(item->objectName()==name)return item;for(auto child:item->childItems())if(auto found=find(child,name))return found;return nullptr;};
    auto click=[&](QQuickItem* item){QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,item->mapToScene(item->boundingRect().center()).toPoint());};
    auto labelFor=[&](QString text)->QQuickItem*{
      auto tabs=find(window->contentItem(),"archiveTabs");if(!tabs)return nullptr;
      auto content=qvariant_cast<QQuickItem*>(tabs->property("contentItem"));if(!content)return nullptr;
      for(auto tab:content->childItems())if(auto label=find(tab,"archiveTabName"))if(label->property("text").toString()==text)return label;
      return nullptr;
    };
    int checked=0;
    for(const auto& locale:api.languages()){
      const auto code=locale.toMap()["code"].toString();QVERIFY2(names.contains(code),qPrintable(code));
      const auto filename=names[code]+" — 0123456789 0123456789 0123456789.iso";
      const auto path=work.path()+'/'+filename;QVERIFY(QFile::copy(fixture,path));
      api.setPreference("language",code);api.openUrls({QUrl::fromLocalFile(path)});
      QTRY_VERIFY_WITH_TIMEOUT(!api.busy()&&api.archiveFormat()=="iso",12000);QCOMPARE(api.tabs().size(),2);
      const auto secondId=api.activeTab();QVERIFY(secondId!=firstId);
      for(const auto& theme:api.themeList()){
        const auto themeId=theme.toMap()["id"].toString();api.setPreference("skin",themeId);QTest::qWait(30);
        auto logo=find(window->contentItem(),"archiveLogo");QVERIFY(logo);QCOMPARE(logo->property("archiveRatio").toDouble(),api.archiveRatio());
        QCOMPARE(logo->property("accentColor").value<QColor>(),QColor(api.colors()["--accent"].toString()));
        auto tabs=find(window->contentItem(),"archiveTabs");auto add=find(window->contentItem(),"archiveTabAdd");QVERIFY(tabs);QVERIFY(add);
        auto content=qvariant_cast<QQuickItem*>(tabs->property("contentItem"));QVERIFY(content);
        QList<QQuickItem*> delegates;for(auto item:content->childItems())if(item->objectName()=="archiveTab")delegates.append(item);
        QTRY_COMPARE(delegates.size(),2);
        for(auto tab:delegates){
          auto label=find(tab,"archiveTabName");auto close=find(tab,"archiveTabClose");QVERIFY(label);QVERIFY(close);
          tab->ensurePolished();QTRY_VERIFY(label->height()+1>=label->property("contentHeight").toDouble());
          const auto center=tab->mapToScene(tab->boundingRect().center()).y();
          const auto context=themeId+'/'+code+'/'+label->property("text").toString();
          QVERIFY2(qAbs(label->mapToScene(label->boundingRect().center()).y()-center)<=1,qPrintable(context));
          QVERIFY2(qAbs(close->mapToScene(close->boundingRect().center()).y()-center)<=1,qPrintable(context));
          QVERIFY2(qAbs(add->mapToScene(add->boundingRect().center()).y()-center)<=1,qPrintable(context));
          const auto labelBounds=label->mapRectToItem(tab,label->boundingRect()),closeBounds=close->mapRectToItem(tab,close->boundingRect());
          QVERIFY2(!labelBounds.intersects(closeBounds),qPrintable(context));
          QVERIFY2(labelBounds.top()>=0&&labelBounds.bottom()<=tab->height()+1,qPrintable(context));
          QVERIFY2(closeBounds.top()>=0&&closeBounds.bottom()<=tab->height()+1,qPrintable(context));
          QVERIFY2(label->property("verticalAlignment").toInt()==Qt::AlignVCenter,qPrintable(context));
          if(label->property("text").toString()!="sample.iso"){QCOMPARE(label->property("text").toString(),filename);QVERIFY(label->property("truncated").toBool());}
        }
        if(QStringList{"en","zh-CN","ar","hi","th"}.contains(code)){
          QVERIFY(QDir().mkpath("tab-layout-captures"));
          QVERIFY(window->grabWindow().save("tab-layout-captures/"+themeId+'-'+code+".png"));
        }
        ++checked;
      }
      // Exercise the real hit targets in both left-to-right and mirrored layouts.
      QTRY_VERIFY(labelFor("sample.iso"));click(labelFor("sample.iso"));QTRY_COMPARE(api.activeTab(),firstId);
      // changed() refreshes the ListView model, so reacquire delegate pointers.
      QTest::qWait(20);QTRY_VERIFY(labelFor(filename));click(labelFor(filename));QTRY_COMPARE(api.activeTab(),secondId);
      QTest::qWait(20);QTRY_VERIFY(labelFor(filename));
      auto secondClose=find(labelFor(filename)->parentItem()->parentItem(),"archiveTabClose");
      QVERIFY(secondClose);click(secondClose);QTRY_COMPARE(api.tabs().size(),1);QCOMPARE(api.activeTab(),firstId);
    }
    QCOMPARE(checked,108);QVERIFY2(warnings.isEmpty(),"QML diagnostics emitted during archive tab layout/interaction checks");
    qInfo()<<"108 archive tab layouts: six themes x eighteen languages, mixed-script long filenames, select and close hit targets";
  }
  void settingsLayoutAndSavedSelections(){
    const auto data=work.path()+"/settings-layout";
    { AppController saved(worker,library,nullptr,data); saved.setPreference("language","de"); saved.setPreference("codepage","936"); saved.setPreference("skin","sleep"); }
    AppController api(worker,library,nullptr,data);
    QCOMPARE(api.languageCode(),QString("de")); QCOMPARE(api.property("filenameCodepage").toString(),QString("936")); QCOMPARE(api.themeId(),QString("sleep"));
    QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("backend",&api);engine.rootContext()->setContextProperty("designPreview",false);engine.load(QUrl::fromLocalFile(qml));
    QVERIFY(!engine.rootObjects().isEmpty());auto root=engine.rootObjects().first();auto window=qobject_cast<QQuickWindow*>(root);QVERIFY(window);
    window->resize(880,560);
    auto scene=[&](QString name){QVERIFY(QMetaObject::invokeMethod(root,"openDesignScene",Q_ARG(QVariant,name)));QTest::qWait(20);};
    scene("settings");
    auto codepage=root->findChild<QObject*>("prefCodepage");auto language=root->findChild<QObject*>("prefLanguage");QVERIFY(codepage);QVERIFY(language);
    QTRY_COMPARE(codepage->property("currentValue").toString(),QString("936"));
    QTRY_COMPARE(language->property("currentValue").toString(),QString("de"));
    // Programmatic updates must propagate too, without reopening the window.
    api.setPreference("codepage","932");QTRY_COMPARE(codepage->property("currentValue").toString(),QString("932"));
    const QStringList pages={"settings","vault","appearance","associations","language","about"};
    QStringList failures;int checked=0;QSignalSpy warnings(&engine,&QQmlEngine::warnings);
    std::function<void(QQuickItem*,QQuickItem*,QString)> inspect;
    inspect=[&](QQuickItem* item,QQuickItem* container,QString context){
      if(!item->isVisible())return;
      if(item->property("truncated").toBool())failures<<context+": truncated "+item->property("text").toString();
      // Check horizontal reachability of settings content, including below-fold rows.
      if(item->objectName()=="settingsPageBody"){
        for(auto child:item->childItems())if(child->isVisible()&&child->width()>0){
          if(child->x() < -1 || child->x()+child->width()>item->width()+1)failures<<context+": row outside page width";
        }
      }
      if(item->property("text").isValid()&&item->property("contentWidth").isValid()&&!item->property("echoMode").isValid()){
        // Labels and Pill captions have no intentional horizontal scrolling.
        if(item->property("contentWidth").toDouble()>item->width()+1)failures<<context+": text wider than control: "+item->property("text").toString();
        if(item->property("contentHeight").toDouble()>item->height()+1)failures<<context+": text taller than control ("+QString::number(item->property("contentHeight").toDouble())+">"+QString::number(item->height())+"): "+item->property("text").toString();
      }
      for(auto child:item->childItems())inspect(child,container,context);
    };
    for(const auto& theme:api.themeList())for(const auto& locale:api.languages()){
      const auto themeId=theme.toMap()["id"].toString(),code=locale.toMap()["code"].toString();
      api.setPreference("skin",themeId);api.setPreference("language",code);
      for(const auto& page:pages){
        scene(page);auto dialog=root->findChild<QObject*>("settingsDialog");QVERIFY(dialog);
        auto content=qvariant_cast<QQuickItem*>(dialog->property("contentItem"));QVERIFY(content);
        // Font fallback and Layout polish can complete on separate frames,
        // especially on macOS. Check settled geometry, not an intermediate frame.
        const auto before=failures.size();QElapsedTimer settling;settling.start();
        do {
          failures=failures.mid(0,before);
          std::function<void(QQuickItem*)> polish=[&](QQuickItem* item){for(auto child:item->childItems())polish(child);item->ensurePolished();};
          polish(content);polish(content);
          inspect(content,content,themeId+'/'+code+'/'+page);
          if(failures.size()>before)QTest::qWait(10);
        } while(failures.size()>before&&settling.elapsed()<1000);
        ++checked;
        QCOMPARE(language->property("currentValue").toString(),code);
        QVERIFY(dialog->property("height").toDouble()<=window->height()-39);
        auto checkbox=root->findChild<QObject*>("prefRestoreTabs");QVERIFY(checkbox);
        auto label=qvariant_cast<QQuickItem*>(checkbox->property("contentItem"));auto indicator=qvariant_cast<QQuickItem*>(checkbox->property("indicator"));QVERIFY(label);QVERIFY(indicator);
        const auto padding=label->property(api.rtl()?"rightPadding":"leftPadding").toDouble();
        QVERIFY2(padding>=indicator->width(),"Checkbox text overlaps its indicator in mirrored layout");
      }
    }
    QCOMPARE(checked,648);
    // Both control models must still agree after a real close/reopen.
    auto dialog=root->findChild<QObject*>("settingsDialog");QVERIFY(QMetaObject::invokeMethod(dialog,"close"));scene("language");
    QCOMPARE(language->property("currentValue").toString(),api.languageCode());
    QVERIFY2(failures.isEmpty(),qPrintable(failures.mid(0,25).join('\n')));
    QVERIFY2(warnings.isEmpty(),"QML diagnostics emitted while switching settings pages/languages/themes");
    qInfo()<<"648 settings layouts: six themes x eighteen languages x six pages at 880x560; persisted selections restored";
  }
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
void GuiFlow::adaptiveLogoRendering(){
    QQmlEngine engine;QSignalSpy warnings(&engine,&QQmlEngine::warnings);
    QQuickWindow window;window.resize(256,256);
    QQmlComponent component(&engine,QUrl::fromLocalFile(QFileInfo(qml).absoluteDir().filePath("ZenLogo.qml")));
    QScopedPointer<QObject> object(component.create());QVERIFY2(object,qPrintable(component.errorString()));
    auto logo=qobject_cast<QQuickItem*>(object.data());QVERIFY(logo);
    logo->setParentItem(window.contentItem());logo->setSize(QSizeF(256,256));
    logo->setProperty("surfaceColor",QColor(Qt::white));logo->setProperty("baseColor",QColor(Qt::white));
    for(const auto* key:{"accentColor","strongColor","textColor"})logo->setProperty(key,QColor(Qt::black));
    window.show();
    auto darkPixels=[&]{const auto image=window.grabWindow();int n=0;for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(qGray(image.pixel(x,y))<64)++n;return n;};
    logo->setProperty("archiveRatio",1.0);
    QTRY_VERIFY_WITH_TIMEOUT(double(darkPixels())/(window.grabWindow().width()*window.grabWindow().height())>0.17 && double(darkPixels())/(window.grabWindow().width()*window.grabWindow().height())<0.22,4000);
    const auto full=darkPixels();
    logo->setProperty("archiveRatio",0.6);
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(double(darkPixels())/full-0.6)<0.025,4000);
    QCOMPARE(logo->property("openingFraction").toDouble(),0.4);
    QDir().mkpath("logo-captures");QVERIFY(window.grabWindow().save("logo-captures/retained-60-open-40.png"));
    logo->setProperty("archiveRatio",0.0);QTRY_COMPARE_WITH_TIMEOUT(darkPixels(),0,4000);
    logo->setProperty("archiveRatio",1.2);QTRY_VERIFY_WITH_TIMEOUT(qAbs(double(darkPixels())/full-1)<0.025,4000);
    for(const auto ratio:{-1.0,std::numeric_limits<double>::quiet_NaN()}){
      logo->setProperty("archiveRatio",ratio);QVERIFY(!logo->property("measured").toBool());QVERIFY(qAbs(logo->property("openingFraction").toDouble()-48.0/360)<1e-12);
    }
    const auto data=work.path()+"/logo-themes";AppController api(worker,library,nullptr,data);
    for(const auto& theme:api.themeList()){
      const auto id=theme.toMap()["id"].toString();api.setPreference("skin",id);const auto colors=api.colors();
      logo->setProperty("surfaceColor",QColor(colors["--bg"].toString()));logo->setProperty("baseColor",QColor(colors["--bg"].toString()));
      logo->setProperty("accentColor",QColor(colors["--accent"].toString()));logo->setProperty("strongColor",QColor(colors["--accent-strong"].toString()));logo->setProperty("textColor",QColor(colors["--text"].toString()));
      QCOMPARE(logo->property("foregroundColor").value<QColor>(),QColor(colors["--accent"].toString()));
      logo->setProperty("artwork",QVariantMap{{"pattern","dots"},{"opacity",0.12},{"spacing",12}});
      logo->setProperty("archiveRatio",0.6);QTest::qWait(40);QVERIFY(window.grabWindow().save("logo-captures/"+id+"-60.png"));
    }
    QDir().mkpath(data+"/themes/textures");QImage texture(8,8,QImage::Format_RGB32);texture.fill(Qt::gray);QVERIFY(texture.save(data+"/themes/textures/paper.png"));
    {QFile theme(data+"/themes/textured.json");QVERIFY(theme.open(QIODevice::WriteOnly));theme.write(R"json({"id":"textured","base":"dark","vars":{"--accent":"#9ABDB9"},"background":{"pattern":"grid","texture":"textures/paper.png","opacity":0.12,"spacing":12}})json");}
    api.rescanThemes();api.setPreference("skin","textured");QCOMPARE(api.themeBackground()["pattern"].toString(),QString("grid"));QVERIFY(!api.themeBackground()["texture"].toString().isEmpty());
    logo->setProperty("artwork",api.themeBackground());QTest::qWait(100);QVERIFY(window.grabWindow().save("logo-captures/custom-texture.png"));
    // Imported skins can have poor contrast and translucent foregrounds.
    logo->setProperty("surfaceColor",QColor(Qt::white));logo->setProperty("baseColor",QColor(Qt::white));
    for(const auto* key:{"accentColor","strongColor","textColor"})logo->setProperty(key,QColor("#01FFFFFF"));
    QCOMPARE(logo->property("foregroundColor").value<QColor>(),QColor(Qt::black));
    logo->setSize(QSizeF(20,20));QTRY_VERIFY(logo->property("small").toBool());
    QCOMPARE(warnings.size(),0);
    logo->setParentItem(nullptr);
  }

int main(int argc,char** argv){QGuiApplication app(argc,argv);QQuickStyle::setStyle("Basic");if(app.arguments().size()!=5)return 2;GuiFlow flow;flow.worker=app.arguments()[1];flow.library=app.arguments()[2];flow.archive=app.arguments()[3];flow.qml=app.arguments()[4];auto file=QDir::current().filePath("native-gui-flow.txt");auto output=(file+",txt").toUtf8();char* options[]={argv[0],const_cast<char*>("-o"),output.data()};const auto result=QTest::qExec(&flow,3,options);QFile log(file);if(log.open(QIODevice::ReadOnly))std::cout<<log.readAll().toStdString()<<std::flush;return result;}
#include "gui.moc"
