// Baseline development benchmark of the unmodified Electron implementation.
const started=process.hrtime.bigint();
const {app,BrowserWindow}=require('electron');
const fs=require('fs'),os=require('os'),path=require('path');
app.setPath('userData',fs.mkdtempSync(path.join(os.tmpdir(),'reazip-benchmark-')));
const value=name=>{const i=process.argv.indexOf(name);return i<0?'':process.argv[i+1]};
require('../../main.js');
app.whenReady().then(async()=>{
 const win=BrowserWindow.getAllWindows()[0];
 if(win.webContents.isLoading())await new Promise(resolve=>win.webContents.once('did-finish-load',resolve));
 win.show();
 await new Promise(resolve=>setTimeout(resolve,300));
 await win.webContents.executeJavaScript('new Promise(r=>requestAnimationFrame(()=>requestAnimationFrame(r)))');
 const firstFrameMs=Number(process.hrtime.bigint()-started)/1e6;
 const archive=value('--archive');let archiveReadyMs=-1;
 if(archive){await win.webContents.executeJavaScript(`openArchive(${JSON.stringify(path.resolve(archive))})`);archiveReadyMs=Number(process.hrtime.bigint()-started)/1e6;}
 await new Promise(resolve=>setTimeout(resolve,2000));
 const metrics=app.getAppMetrics();
 fs.writeFileSync(value('--output'),JSON.stringify({version:require('../../package.json').version,electron:process.versions.electron,firstFrameMs,archiveReadyMs,residentBytes:String(metrics.reduce((sum,item)=>sum+item.memory.workingSetSize*1024,0)),processCount:metrics.length,scene:archive?'archive':'empty',measurement:'Application-entry timing; summed process working sets, warm OS caches'},null,2)+'\n');
 app.exit(0);
}).catch(error=>{console.error(error);app.exit(1)});
