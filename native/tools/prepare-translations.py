"""Merge reviewed native labels with the original 18 JSON dictionaries and audit keys."""
import json, pathlib, re
root = pathlib.Path(__file__).resolve().parents[1]
labels = json.loads((root/'tools/translations-native.json').read_text(encoding='utf-8'))
keys = ['encryptHeaders','confirmOpen','confirmExecutable','prepareDrag','dragSelected','themeCompatibility','platformIntegration','encryption']
aliases = {'tempFailed':'failed','invalidDirectory':'failed','settingsWriteFailed':'failed','invalidOutput':'failed','registeredChooseDefault':'setDefault'}
for state, key in {'Running':'loading','Committing':'loading','AwaitingPassword':'needPassword','AwaitingOverwrite':'conflictTitle','Cancelling':'cancelTask','Succeeded':'done','Idle':'ready','Failed':'failed','Cancelled':'cancelled'}.items():
    aliases['state'+state] = key
for error in ['UnsupportedFormat','UnsupportedOperation','EngineMissing','EngineArchitectureMismatch','PermissionDenied','CorruptArchive','UnsafeEntry','IoError','WorkerMissing','WorkerCrashed','ProtocolError']:
    aliases['error'+error] = 'failed'
aliases.update(errorPasswordRequired='needPassword', errorWrongPassword='passwordWrong',errorCancelled='cancelled')
catalogues = {}
for language, values in labels.items():
    path = root/'i18n'/f'{language}.json'
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    data.update(zip(keys,values))
    for key, original in aliases.items(): data[key] = data[original]
    catalogues[language] = data
    path.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
required = set(catalogues['en'])
used = set()
for path in [root/'ui/Main.qml',root/'ui/Sheet.qml',root/'app/AppController.cpp']:
    used.update(re.findall(r'(?:\bt|\btranslate)\("([A-Za-z0-9_-]+)"\s*,',path.read_text(encoding='utf-8')))
report = {'languageCount':len(catalogues),'keys':len(required),'missing':{language:sorted((required|used)-set(data)) for language,data in catalogues.items()},'genericErrorLabels':sorted(aliases),'notes':['Technical error codes accompany localized generic errors.','Native labels translated for 18 languages; typography and translation quality still need visual QA.']}
(root/'docs/translation-audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
if any(report['missing'].values()): raise SystemExit('Missing translation keys: '+str(report['missing']))
print(f'{len(catalogues)} languages, {len(required)} keys, zero missing keys')
