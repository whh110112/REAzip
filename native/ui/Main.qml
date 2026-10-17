import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
ApplicationWindow {
    id: window
    width: 1120; height: 740; minimumWidth: 880; minimumHeight: 560
    visible: true; title: "REAzip"
    flags: Qt.platform.os === "osx" ? Qt.Window : Qt.Window | Qt.FramelessWindowHint
    property var api: typeof backend !== "undefined" ? backend : null
    property bool preview: typeof designPreview !== "undefined" && designPreview
    property var colors: api ? api.colors : ({})
    property bool hasArchive: api ? api.hasArchive : preview
    property var inputs: []
    property var currentChallenge: ({})
    property string folderTarget: "extract"
    property string toastText: ""
    property string executablePath: ""
    function t(key, fallback) { if (api) { let epoch = api.revision; return api.translate(key, fallback) } return fallback }
    function token(key, fallback) { return colors[key] || fallback }
    function openDesignScene(scene) {
        if(scene==="create")createDialog.open();else if(scene==="extract")extractDialog.open();else if(scene==="settings")settingsDialog.open();else if(scene==="appearance"){settingsPages.currentIndex=1;settingsDialog.open()}else if(scene==="password")passwordDialog.open();else if(scene==="conflict"){window.currentChallenge={path:"example.txt"};conflictDialog.open()}else if(scene==="progress")progressDialog.open()
    }
    readonly property color paper: token("--bg", "#FFFFFF")
    readonly property color surface: token("--surface", "#F1EFE8")
    readonly property color card: token("--card", "#FFFFFF")
    readonly property color ink: token("--text", "#2C2C2A")
    readonly property color muted: token("--muted", "#5F5E5A")
    readonly property color edge: token("--border", "#D3D1C7")
    readonly property color accent: token("--accent", "#378ADD")
    color: paper; font.pixelSize: 13
    LayoutMirroring.enabled: api ? api.rtl : false
    LayoutMirroring.childrenInherit: true
    component Action: Pill { accent: window.accent; ink: window.muted; paper: window.card; edge: window.edge }
    component Field: TextField {
        color: window.ink; selectByMouse: true; implicitHeight: 36; font.pixelSize: 13
        background: Rectangle { radius: 8; color: window.card; border.color: window.edge }
    }
    component Caption: Label { color: window.muted; font.pixelSize: 13 }
    ListModel { id: previewEntries
        ListElement { entryId: "demo:dir:assets"; name: "assets"; directory: true; displaySize: "—"; displayPacked: "—"; kind: "DIR"; modified: "—"; selected: false }
        ListElement { entryId: "demo:dir:docs"; name: "docs"; directory: true; displaySize: "—"; displayPacked: "—"; kind: "DIR"; modified: "—"; selected: false }
        ListElement { entryId: "demo:2"; name: "readme.md"; directory: false; displaySize: "51 B"; displayPacked: "51 B"; kind: "MD"; modified: "2026-10-17"; selected: false }
    }
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.fillWidth: true; implicitHeight: Qt.platform.os === "osx" ? 0 : 36; visible: height > 0; color: window.surface
            MouseArea { anchors.fill: parent; onPressed: window.startSystemMove(); onDoubleClicked: window.visibility === Window.Maximized ? window.showNormal() : window.showMaximized() }
            Row { anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; spacing: 7
                Rectangle { width: 14; height: 14; radius: 4; color: window.accent; Rectangle { x: 8; y: 8; width: 8; height: 8; radius: 2; border.color: window.accent; color: window.card } }
                Label { text: "REAzip"; color: window.ink; font.pixelSize: 12 }
            }
            Label { anchors.centerIn: parent; text: window.hasArchive ? (window.api ? window.api.archiveName : "demo.zip · design preview") : "REAzip"; color: window.muted; font.pixelSize: 12 }
            Row { anchors.right: parent.right; height: parent.height
                ToolButton { width: 46; height: parent.height; text: "−"; onClicked: window.showMinimized(); Accessible.name: "Minimize" }
                ToolButton { width: 46; height: parent.height; text: "□"; onClicked: window.visibility === Window.Maximized ? window.showNormal() : window.showMaximized(); Accessible.name: "Maximize" }
                ToolButton { width: 46; height: parent.height; text: "×"; onClicked: window.close(); Accessible.name: "Close" }
            }
        }
        Rectangle { Layout.fillWidth: true; implicitHeight: 36; color: window.surface
            RowLayout { anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10; spacing: 3
                ListView { Layout.preferredWidth: Math.min(contentWidth, window.width - 60); Layout.fillHeight: true; orientation: ListView.Horizontal; spacing: 3; clip: true
                    model: window.api ? window.api.tabs : (window.preview ? [{id:"demo",name:"demo.zip"}] : [])
                    delegate: Rectangle { required property var modelData; width: Math.min(220, tabName.implicitWidth + 52); height: 30; y: 6; radius: 9; color: window.api && window.api.activeTab !== modelData.id ? window.surface : window.paper; border.color: window.edge
                        RowLayout { anchors.fill: parent; anchors.leftMargin: 10
                            Label { id: tabName; text: modelData.name; color: window.ink; elide: Text.ElideRight; Layout.fillWidth: true }
                            ToolButton { text:"×"; implicitWidth:24; onClicked:if(window.api)window.api.closeTab(modelData.id); Accessible.name:"Close archive" }
                        }
                        TapHandler { onTapped: if(window.api)window.api.activateTab(modelData.id) }
                    }
                }
                Action { text:"+";implicitWidth:28;implicitHeight:24;onClicked:openDialog.open() }
                Item { Layout.fillWidth: true }
            }
        }
        Rectangle { Layout.fillWidth:true;implicitHeight:56;color:window.paper
            RowLayout { anchors.fill:parent;anchors.leftMargin:14;anchors.rightMargin:14;spacing:8
                Action { text:window.t("newArchive","新建压缩");primary:true;enabled:!!window.api;onClicked:createDialog.open() }
                Action { text:window.t("extractTo","解压到");enabled:window.hasArchive&&!!window.api;onClicked:extractDialog.open() }
                Action { text:window.t("test","测试");enabled:window.hasArchive&&!!window.api;onClicked:window.api.testArchive() }
                Action { text:window.t("add","添加");enabled:window.api?window.api.canModify:false;onClicked:addDialog.open() }
                Action { text:window.t("delete","删除");enabled:window.api?window.api.hasSelection&&window.api.canModify:false;onClicked:deleteDialog.open() }
                Item { Layout.fillWidth:true }
                Field { Layout.preferredWidth:Math.min(240,window.width*0.23);placeholderText:window.t("searchPlaceholder","搜索当前压缩包内文件");onTextChanged:if(window.api)window.api.setFilter(text) }
                Action { text:window.t("settings","设置");onClicked:settingsDialog.open() }
            }
            Rectangle { anchors.bottom:parent.bottom;height:1;width:parent.width;color:window.edge }
        }
        RowLayout { Layout.fillWidth:true;Layout.preferredHeight:36;Layout.leftMargin:14;Layout.rightMargin:14
            ToolButton { text:"↑";enabled:window.hasArchive;onClicked:if(window.api)window.api.parentFolder();Accessible.name:"Parent folder" }
            Label { text:window.hasArchive?(window.api?window.api.breadcrumb:"demo.zip"):"";color:window.ink;elide:Text.ElideMiddle;Layout.fillWidth:true;font.bold:true }
            Action { text:window.t("rename","重命名");visible:window.api?window.api.hasSelection&&window.api.canModify:false;onClicked:renameDialog.open() }
            Action { text:window.t("prepareDrag","准备拖出");visible:window.api?window.api.hasSelection&&!window.api.dragPrepared:false;enabled:window.api?!window.api.busy:false;onClicked:window.api.prepareDrag() }
            Action { text:window.t("dragSelected","拖出选中项");visible:window.api?window.api.dragPrepared:false
                MouseArea { anchors.fill:parent;property real startX;property real startY;onPressed:function(mouse){startX=mouse.x;startY=mouse.y};onPositionChanged:function(mouse){if(pressed&&Math.abs(mouse.x-startX)+Math.abs(mouse.y-startY)>10)window.api.beginPreparedDrag()} }
            }
        }
        RowLayout { Layout.fillWidth:true;Layout.fillHeight:true;Layout.leftMargin:14;Layout.rightMargin:14;Layout.bottomMargin:14;spacing:14
            Rectangle { Layout.fillWidth:true;Layout.minimumWidth:400;Layout.fillHeight:true;color:window.paper;border.color:window.edge;radius:10;clip:true
                ColumnLayout { anchors.fill:parent;spacing:0;visible:window.hasArchive
                    Rectangle { Layout.fillWidth:true;implicitHeight:32;color:window.surface
                        Row { anchors.fill:parent;anchors.leftMargin:14
                            Repeater { model:[window.t("name","名称"),window.t("size","大小"),window.t("packed","压缩后"),window.t("type","类型"),window.t("modified","修改时间")]
                                delegate:Item { required property int index;required property string modelData;width:(parent.width-14)*[0.40,0.14,0.14,0.13,0.19][index];height:32
                                    Label { anchors.verticalCenter:parent.verticalCenter;text:modelData;color:window.muted }
                                    TapHandler { onTapped:if(window.api)window.api.sortBy(index) }
                                }
                            }
                        }
                    }
                    ListView { id:files;Layout.fillWidth:true;Layout.fillHeight:true;clip:true;model:window.api?window.api.entries:(window.preview?previewEntries:null);boundsBehavior:Flickable.StopAtBounds
                        ScrollBar.vertical:ScrollBar {}
                        delegate:Rectangle { required property string entryId;required property string name;required property bool directory;required property string displaySize;required property string displayPacked;required property string kind;required property string modified;required property bool selected
                            width:files.width;height:40;color:selected?window.token("--accent-soft","#E6F1FB"):window.paper
                            Row { anchors.fill:parent;anchors.leftMargin:14
                                RowLayout { width:(parent.width-14)*0.40;height:40;spacing:8
                                    Rectangle { implicitWidth:24;implicitHeight:24;radius:7;color:directory?window.token("--accent-soft","#E6F1FB"):window.surface;Text { anchors.centerIn:parent;text:kind;font.pixelSize:8;color:directory?window.accent:window.muted } }
                                    Label { text:name;color:window.ink;elide:Text.ElideRight;Layout.fillWidth:true }
                                }
                                Label { width:(parent.width-14)*0.14;height:40;verticalAlignment:Text.AlignVCenter;text:displaySize;color:window.muted }
                                Label { width:(parent.width-14)*0.14;height:40;verticalAlignment:Text.AlignVCenter;text:displayPacked;color:window.accent }
                                Label { width:(parent.width-14)*0.13;height:40;verticalAlignment:Text.AlignVCenter;text:directory?"DIR":"FILE";color:window.muted }
                                Label { width:(parent.width-14)*0.19;height:40;verticalAlignment:Text.AlignVCenter;text:modified;color:window.muted;elide:Text.ElideRight }
                            }
                            Rectangle { anchors.bottom:parent.bottom;width:parent.width;height:1;color:window.edge;opacity:0.55 }
                            MouseArea { anchors.fill:parent;onClicked:function(mouse){if(window.api)window.api.selectEntry(entryId,!!(mouse.modifiers&(Qt.ControlModifier|Qt.MetaModifier)))};onDoubleClicked:if(window.api)window.api.activateEntry(entryId) }
                        }
                    }
                }
                ColumnLayout { anchors.centerIn:parent;spacing:16;visible:!window.hasArchive
                    Label { text:"⇣";font.pixelSize:48;color:window.accent;Layout.alignment:Qt.AlignHCenter }
                    Label { text:window.t("dropTitle","拖入压缩包，或点击打开");font.pixelSize:19;color:window.ink;Layout.alignment:Qt.AlignHCenter }
                    Label { text:"ZIP · 7Z · RAR · ISO · TAR · GZ · WIM";color:window.muted;Layout.alignment:Qt.AlignHCenter }
                    Action { text:window.t("openArchive","打开压缩包");primary:true;Layout.alignment:Qt.AlignHCenter;onClicked:openDialog.open() }
                }
                DropArea { anchors.fill:parent;onDropped:function(drop){if(window.api&&drop.hasUrls)window.api.openUrls(drop.urls)} }
            }
            ColumnLayout { Layout.preferredWidth:186;Layout.minimumWidth:186;Layout.maximumWidth:186;Layout.fillHeight:true;spacing:12;visible:window.hasArchive
                Caption { text:window.api?window.api.archiveFormat.toUpperCase():"ZIP";Layout.topMargin:16 }
                Repeater { model:window.api?window.api.details:[{label:"大小",value:"1.9 KB"},{label:"压缩后",value:"1.2 KB"},{label:"文件",value:"5"},{label:"文件夹",value:"2"},{label:"加密",value:"none"}]
                    delegate:RowLayout { required property var modelData;Layout.fillWidth:true;Caption { text:modelData.label;Layout.fillWidth:true } Label { text:modelData.value;color:window.ink } }
                }
                Action { text:window.t("test","测试完整性");Layout.fillWidth:true;enabled:!!window.api;onClicked:window.api.testArchive() }
                Item { Layout.fillHeight:true }
            }
        }
        Rectangle { Layout.fillWidth:true;implicitHeight:30;color:window.surface
            RowLayout { anchors.fill:parent;anchors.leftMargin:14;anchors.rightMargin:14
                Caption { text:window.api?window.api.statusSummary:(window.preview?"5 文件    2 文件夹    1.2 KB":"");font.pixelSize:12 }
                Item { Layout.fillWidth:true }
                Label { text:window.api?window.api.status:window.t("ready","就绪");color:window.token("--ok","#1D9E75");font.pixelSize:12 }
            }
        }
    }
    FileDialog { id:openDialog;title:window.t("openArchive","打开压缩包");fileMode:FileDialog.OpenFiles;onAccepted:if(window.api)window.api.openUrls(selectedFiles) }
    FileDialog { id:sourcesDialog;fileMode:FileDialog.OpenFiles;onAccepted:window.inputs=selectedFiles }
    FileDialog { id:addDialog;fileMode:FileDialog.OpenFiles;onAccepted:if(window.api)window.api.addUrls(selectedFiles) }
    FolderDialog { id:folders;onAccepted:{let path=window.api?window.api.localPath(selectedFolder):selectedFolder;if(window.folderTarget==="source")window.inputs=[selectedFolder];else if(window.folderTarget==="create")createDest.text=path;else extractDest.text=path} }
    Sheet { id:createDialog;title:window.t("newArchive","新建压缩");paper:window.card;ink:window.ink;edge:window.edge
        ColumnLayout { width:parent.width;spacing:10
            Caption { text:window.t("fileName","文件名") }
            Field { id:createName;text:"archive.zip";Layout.fillWidth:true }
            Caption { text:window.t("saveTo","保存到") }
            RowLayout { Field { id:createDest;text:window.api?window.api.defaultArchiveDir:"";Layout.fillWidth:true } Action { text:"…";onClicked:{window.folderTarget="create";folders.open()} } }
            RowLayout { ComboBox { id:format;model:["zip","7z","tar","tgz"] } ComboBox { id:level;model:[0,1,3,5,7,9];currentIndex:3 } Field { id:volume;placeholderText:window.t("volume","分卷大小")+" (B)";enabled:format.currentText!=="tgz";Layout.fillWidth:true } }
            Field { id:createPassword;placeholderText:window.t("passwordOptional","密码（可选）");echoMode:TextInput.Password;enabled:format.currentIndex<2;Layout.fillWidth:true }
            CheckBox { id:headers;text:window.t("encryptHeaders","7Z 文件头加密");enabled:format.currentText==="7z";palette.text:window.ink }
            CheckBox { id:deleteSources;text:window.t("deleteSource","成功校验后将源文件移至回收站");palette.text:window.ink }
            Caption { text:window.inputs.length+" "+window.t("files","文件/目录") }
            RowLayout { Action { text:window.t("addFiles","选择文件");onClicked:sourcesDialog.open() } Action { text:window.t("addFolder","选择目录");onClicked:{window.folderTarget="source";folders.open()} } }
        }
        footer:RowLayout { Item { Layout.fillWidth:true } Action { text:window.t("cancel","取消");onClicked:createDialog.reject() } Action { text:window.t("startCompress","开始压缩");primary:true;enabled:!!window.api&&window.inputs.length>0;onClicked:{window.api.createArchive({name:createName.text,destination:createDest.text,inputs:window.inputs,format:format.currentText,level:level.currentValue,password:createPassword.enabled?createPassword.text:"",encryptHeaders:headers.enabled&&headers.checked,deleteSources:deleteSources.checked,volumeSize:volume.enabled?volume.text:""});createPassword.clear();createDialog.close()} } }
    }
    Sheet { id:extractDialog;title:window.t("extractTo","解压到");paper:window.card;ink:window.ink;edge:window.edge
        ColumnLayout { width:parent.width;spacing:12
            Caption { text:window.t("saveTo","目标目录") }
            RowLayout { Field { id:extractDest;text:window.api?window.api.defaultExtractDir:"";Layout.fillWidth:true } Action { text:"…";onClicked:{window.folderTarget="extract";folders.open()} } }
            CheckBox { id:smart;text:window.t("smartExtract","智能选择同名目录");checked:true;palette.text:window.ink }
            ComboBox { id:overwrite;textRole:"label";valueRole:"value";model:[{label:window.t("ask","询问"),value:"ask"},{label:window.t("overwrite","覆盖"),value:"overwrite"},{label:window.t("skip","跳过"),value:"skip"},{label:window.t("rename","重命名"),value:"rename"}] }
            CheckBox { id:openAfter;text:window.t("openAfter","完成后打开目录");checked:true;palette.text:window.ink }
        }
        footer:RowLayout { Item { Layout.fillWidth:true } Action { text:window.t("cancel","取消");onClicked:extractDialog.reject() } Action { text:window.t("startExtract","开始解压");primary:true;onClicked:{if(window.api)window.api.extractArchive({destination:extractDest.text,smart:smart.checked,overwrite:overwrite.currentValue,openAfter:openAfter.checked});extractDialog.close()} } }
    }
    Sheet { id:passwordDialog;title:window.t("needPassword","需要密码");paper:window.card;ink:window.ink;edge:window.edge
        ColumnLayout { width:parent.width;Caption { text:window.currentChallenge.incorrect?window.t("passwordWrong","密码错误，请重试"):window.t("enterPassword","请输入归档密码") } Field { id:password;echoMode:TextInput.Password;Layout.fillWidth:true;onAccepted:passwordDialog.accept() } }
        standardButtons:Dialog.Ok|Dialog.Cancel
        onOpened:{password.clear();password.forceActiveFocus()}
        onAccepted:{if(window.api)window.api.answerChallenge(window.currentChallenge.jobId,window.currentChallenge.challengeId,password.text);password.clear()}
        onRejected:{password.clear();if(window.api)window.api.cancelTask()}
    }
    Sheet { id:executableDialog;title:window.t("confirmOpen","打开可执行文件");paper:window.card;ink:window.ink;edge:window.edge
        Caption { width:parent.width;text:window.t("confirmExecutable","此归档条目可能运行程序或脚本。确定打开？")+"\n"+window.executablePath;wrapMode:Text.WrapAnywhere }
        standardButtons:Dialog.Ok|Dialog.Cancel
        onAccepted:if(window.api)window.api.openExternal(window.executablePath)
    }
    Sheet { id:conflictDialog;title:window.t("conflictTitle","目标位置已有同名文件");paper:window.card;ink:window.ink;edge:window.edge
        Caption { width:parent.width;text:window.currentChallenge.path||"";wrapMode:Text.WrapAnywhere }
        footer:RowLayout { Action { text:window.t("cancel","取消");onClicked:conflictDialog.reject() } Repeater { model:[{label:window.t("skip","跳过"),value:"skip"},{label:window.t("rename","重命名"),value:"rename"},{label:window.t("overwrite","覆盖"),value:"overwrite"}];delegate:Action { required property var modelData;text:modelData.label;onClicked:{if(window.api)window.api.answerChallenge(window.currentChallenge.jobId,window.currentChallenge.challengeId,modelData.value);conflictDialog.close()} } } }
        onRejected:if(window.api)window.api.cancelTask()
    }
    Sheet { id:progressDialog;title:window.t("loading","处理中");paper:window.card;ink:window.ink;edge:window.edge;closePolicy:Popup.NoAutoClose
        ColumnLayout { width:parent.width;Label { text:Math.round(window.api?window.api.progress:0)+"%";color:window.ink;font.pixelSize:32;Layout.alignment:Qt.AlignHCenter } ProgressBar { Layout.fillWidth:true;value:window.api?window.api.progress/100:0 } Caption { text:window.api?window.t("state"+window.api.jobState,window.api.jobState):"" } }
        footer:Action { text:window.t("cancelTask","取消任务");onClicked:if(window.api)window.api.cancelTask() }
    }
    Sheet { id:deleteDialog;title:window.t("delete","删除");paper:window.card;ink:window.ink;edge:window.edge;standardButtons:Dialog.Ok|Dialog.Cancel;Caption { text:window.t("deleteConfirm","确认删除所选归档条目？") } onAccepted:if(window.api)window.api.deleteSelected() }
    Sheet { id:renameDialog;title:window.t("rename","重命名");paper:window.card;ink:window.ink;edge:window.edge;standardButtons:Dialog.Ok|Dialog.Cancel;Field { id:renameText;width:parent.width;text:window.api?window.api.selectedName:"" } onAccepted:if(window.api)window.api.renameSelected(renameText.text) }
    Sheet { id:settingsDialog;title:window.t("settings","设置");width:Math.min(780,window.width-40);paper:window.card;ink:window.ink;edge:window.edge
        RowLayout { width:parent.width;spacing:18
            ColumnLayout { Layout.preferredWidth:135;Layout.alignment:Qt.AlignTop;Repeater { model:[window.t("tabGeneral","常规"),window.t("tabAppearance","外观皮肤"),window.t("tabAssoc","文件关联"),window.t("tabLanguage","语言"),window.t("tabAbout","关于")];delegate:Action { required property int index;required property string modelData;text:modelData;Layout.fillWidth:true;primary:settingsPages.currentIndex===index;onClicked:settingsPages.currentIndex=index } } }
            StackLayout { id:settingsPages;Layout.fillWidth:true;Layout.preferredHeight:300
                ColumnLayout { Caption { text:window.t("defaultExtractDir","默认解压位置") } Field { text:window.api?window.api.defaultExtractDir:"";Layout.fillWidth:true;onEditingFinished:if(window.api)window.api.setPreference("extractDir",text) } Caption { text:window.t("defaultArchiveDir","默认压缩位置") } Field { text:window.api?window.api.defaultArchiveDir:"";Layout.fillWidth:true;onEditingFinished:if(window.api)window.api.setPreference("archiveDir",text) } CheckBox { text:window.t("restoreTabs","启动时恢复上次标签");checked:window.api?window.api.restoreTabs:false;palette.text:window.ink;onToggled:if(window.api)window.api.setPreference("restoreTabs",checked) } ComboBox { model:["Auto","936","950","932","65001"];onActivated:if(window.api)window.api.setPreference("codepage",currentIndex===0?"":currentText) } Item { Layout.fillHeight:true } }
                ColumnLayout { GridLayout { columns:2;Repeater { model:window.api?window.api.themeList:[];delegate:Action { required property var modelData;text:modelData.name;Layout.fillWidth:true;onClicked:if(window.api)window.api.setPreference("skin",modelData.id) } } } Action { text:window.t("openSkinDir","打开皮肤目录");onClicked:if(window.api)window.api.openThemeDirectory() } Action { text:window.t("rescan","重新扫描");onClicked:if(window.api)window.api.rescanThemes() } Caption { text:window.t("themeCompatibility","JSON 颜色变量兼容；追加 CSS 不执行");wrapMode:Text.Wrap;Layout.fillWidth:true } Item { Layout.fillHeight:true } }
                ColumnLayout { Caption { text:"ZIP · 7Z · RAR · TAR · GZ · ISO" } Caption { text:window.t("platformIntegration","系统集成状态以平台实现为准");wrapMode:Text.Wrap;Layout.fillWidth:true } Action { text:window.t("applyDefault","注册关联并打开默认应用设置");enabled:!!window.api;onClicked:window.api.applyAssociations() } Item { Layout.fillHeight:true } }
                ColumnLayout { Caption { text:window.t("uiLanguage","界面语言") } ComboBox { model:window.api?window.api.languages:[];textRole:"name";onActivated:if(window.api)window.api.setPreference("language",model[currentIndex].code) } Caption { text:window.t("langHint","支持18种语言") } Item { Layout.fillHeight:true } }
                ColumnLayout { Label { text:"REAzip "+(window.api?window.api.version:"design preview");color:window.ink;font.pixelSize:20 } Caption { text:window.api?window.api.engineIdentity:"C++17 · Qt Quick · 7-Zip native library";wrapMode:Text.Wrap;Layout.fillWidth:true } Caption { text:"github.com/whh110112/REAzip" } Item { Layout.fillHeight:true } }
            }
        }
        footer:Action { text:window.t("done","完成");onClicked:settingsDialog.close() }
    }
    Popup { id:toast;anchors.centerIn:parent;width:Math.min(600,window.width-40);background:Rectangle{color:window.ink;radius:10} Label{width:parent.width;text:window.toastText;color:window.paper;wrapMode:Text.Wrap} Timer{id:toastTimer;interval:4000;onTriggered:toast.close()} }
    Connections { target:window.api;ignoreUnknownSignals:true
        function onMessage(text,error){window.toastText=text;toast.open();toastTimer.restart()}
        function onExecutableRequested(path){window.executablePath=path;executableDialog.open()}
        function onCreateRequested(paths){window.inputs=paths;createDialog.open()}
        function onExtractRequested(){extractDialog.open()}
        function onChallenge(jobId,challengeId,kind,details){window.currentChallenge=details;window.currentChallenge.jobId=jobId;window.currentChallenge.challengeId=challengeId;progressDialog.close();if(kind==="passwordRequired")passwordDialog.open();else conflictDialog.open()}
        function onJobChanged(){if(window.api.busy&&window.api.jobState!=="AwaitingPassword"&&window.api.jobState!=="AwaitingOverwrite")progressDialog.open();else progressDialog.close()}
    }
    Shortcut { sequence:StandardKey.Open;onActivated:openDialog.open() }
    Shortcut { sequence:StandardKey.New;enabled:!!window.api;onActivated:createDialog.open() }
    Shortcut { sequence:Qt.platform.os==="osx"?"Meta+E":"Ctrl+E";enabled:window.hasArchive;onActivated:extractDialog.open() }
    Shortcut { sequence:StandardKey.Preferences;onActivated:settingsDialog.open() }
    Repeater { model:Qt.platform.os==="osx"?[]:[{edges:Qt.LeftEdge|Qt.TopEdge,x:0,y:0,w:6,h:6},{edges:Qt.RightEdge|Qt.TopEdge,x:window.width-6,y:0,w:6,h:6},{edges:Qt.LeftEdge|Qt.BottomEdge,x:0,y:window.height-6,w:6,h:6},{edges:Qt.RightEdge|Qt.BottomEdge,x:window.width-6,y:window.height-6,w:6,h:6},{edges:Qt.LeftEdge,x:0,y:6,w:4,h:window.height-12},{edges:Qt.RightEdge,x:window.width-4,y:6,w:4,h:window.height-12},{edges:Qt.TopEdge,x:6,y:0,w:window.width-12,h:4},{edges:Qt.BottomEdge,x:6,y:window.height-4,w:window.width-12,h:4}]
        delegate:MouseArea { required property var modelData;x:modelData.x;y:modelData.y;width:modelData.w;height:modelData.h;z:1000;onPressed:window.startSystemResize(modelData.edges) }
    }
}
