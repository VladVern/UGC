const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
    readFile: (filePath) =>
        ipcRenderer.invoke('file:read', filePath),

    writeFile: (filePath, content) =>
        ipcRenderer.invoke('file:write', filePath, content),

    launchExe: () =>
        ipcRenderer.invoke('app:launch-exe')
})