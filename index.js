const { app, BrowserWindow, ipcMain } = require('electron')
const { spawn } = require('node:child_process')
const fs = require('node:fs/promises')
const path = require('node:path')

const createWindow = () => {
    const win = new BrowserWindow({
        width: 1000,
        height: 700,
        resizable: false,
        icon: path.join(__dirname, './icon.png'),
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            contextIsolation: true,
            nodeIntegration: false,
            devTools: false
        }
    })
    win.setMenu(null)

    win.loadFile(path.join(__dirname, 'site', 'index.html'))
}

ipcMain.handle('file:read', async (_, filePath) => {
    return await fs.readFile(filePath, 'utf-8')
})
ipcMain.handle('file:write', async (_, filePath, content) => {
    await fs.writeFile(filePath, content, 'utf-8')
    return true
})

ipcMain.handle('app:launch-exe', async () => {
    const exePath = app.isPackaged
        ? path.join(path.dirname(process.execPath), 'commander.exe')
        : path.join(__dirname, 'commander.exe')

    const cmdPath = path.join(
        process.env.SystemRoot || 'C:\\Windows',
        'System32',
        'cmd.exe'
    )

    await fs.access(exePath)
    await fs.access(cmdPath)

    return new Promise((resolve, reject) => {
        const child = spawn(cmdPath, [
            '/c',
            'start',
            'Commander',
            '""',
            `"${exePath}"`
        ], {
            cwd: path.dirname(exePath),
            detached: true,
            stdio: 'ignore',
            windowsHide: true
        })

        child.once('error', reject)

        child.once('spawn', () => {
            child.unref()
            resolve(true)
            app.quit()
        })
    })
})

app.whenReady().then(createWindow)