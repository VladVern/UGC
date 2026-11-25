// Create Save config
async function readConfig() {
    try {
        const content = await window.electronAPI.readFile('./config/config.json')
        const config = JSON.parse(content)

        return config
    } catch (error) {
        console.error(error)

        return {
            presets: [],
            "selected-preset": 'Error not found config'
        }
    }
}
async function saveConfig(config) {
    try {
        await window.electronAPI.writeFile('./config/config.json', JSON.stringify(config, null, 4))
    } catch (error) {
        console.error(error)
    }
}



// Render
function renderPresets() {
    const presets = document.querySelector('.conf-preset-select')
    presets.innerHTML = ''

    if (!config.presets.length) {
        presets.innerHTML = `
            <h1 class="conf-preset-select-null">There are no presets</h1>
        `
        return
    }

    config.presets.forEach((preset, index) => {
        const el = document.createElement('div')
        el.classList.add('conf-preset-select-item')

        const title = document.createElement('h1')
        title.textContent = preset.name

        const deleteButton = document.createElement('button')
        deleteButton.classList.add('conf-preset-select-item-delete')
        deleteButton.dataset.index = index
        deleteButton.innerHTML = `
            <img draggable="false" src="./assets/img/trash.svg" alt="Delete">
        `

        const selectButton = document.createElement('button')
        selectButton.classList.add('conf-preset-select-item-select')
        selectButton.dataset.index = index
        selectButton.innerHTML = `
            <img draggable="false" src="./assets/img/chevron-up.svg" alt="Select">
        `

        el.append(title, deleteButton, selectButton)
        presets.appendChild(el)
    })

    presets.querySelectorAll('.conf-preset-select-item-select').forEach(button => {
        button.addEventListener('click', () => {
            const index = Number(button.dataset.index)
            const preset = config.presets[index]

            if (!preset) return

            config["selected-preset"] = preset.name

            renderSelectedPreset()
        })
    })

    presets.querySelectorAll('.conf-preset-select-item-delete').forEach(button => {
        button.addEventListener('click', () => {
            const index = Number(button.dataset.index)

            if (!config.presets[index]) return

            const deletedPreset = config.presets[index]

            if (deletedPreset.name === config["selected-preset"]) {
                config["selected-preset"] = ''
            }

            config.presets.splice(index, 1)

            renderPresets()
            renderSelectedPreset()
        })
    })
}
function renderSelectedPreset() {
    const title = document.querySelector('.conf-preset span')

    title.textContent = config?.["selected-preset"] || 'none'

    renderEditPreset()
}
function renderEditCommands() {
    const edit = document.querySelector('.conf-open-edit-commands-commands-container')

    if (!edit) return

    edit.innerHTML = ''

    const preset = config.presets.find(preset => preset.name === config["selected-preset"])

    if (!preset || !preset.commands?.length) return

    preset.commands.forEach((command, index) => {
        const item = document.createElement('div')
        item.classList.add('conf-open-edit-commands-commands-item')

        const commandName = document.createElement('h1')
        commandName.textContent = command.command

        const commandKey = document.createElement('h1')
        commandKey.textContent = command.key

        const deleteButton = document.createElement('button')
        deleteButton.draggable = false
        deleteButton.innerHTML = `
            <img src="./assets/img/trash.svg" alt="Delete">
        `

        deleteButton.addEventListener('click', () => {
            preset.commands.splice(index, 1)
            renderEditCommands()
        })

        item.append(commandName, commandKey, deleteButton)
        edit.appendChild(item)
    })
}
function renderEditPreset() {
    const editMain = document.querySelector('.conf-open-edit-main')
    editMain.innerHTML = ''

    const preset = config.presets.find(preset => preset.name === config["selected-preset"])

    if (!preset) {
        document.querySelector('.conf-open-edit-commands').innerHTML = ''
        return
    }

    editMain.innerHTML = `
        <div class="conf-open-edit-main-title">
            <p>Preset name</p>
            <p>Cooldown ms (-1 none)</p>
            <p>Open chat key</p>
            <p>Input chat key</p>
            <p>Close chat key</p>
            <p>Closing chat?</p>
        </div>

        <div class="conf-open-edit-main-line"></div>

        <div class="conf-open-edit-main-inputs">
            <input id="inputName" placeholder="Preset name (enter)" type="text">
            <input id="inputCooldown" placeholder="Cooldown ms (-1 none)" type="number">
            <input id="inputOpen" placeholder="Open chat key" type="text">
            <input id="inputInput" placeholder="Input chat key" type="text">
            <input id="inputClose" placeholder="Close chat key" type="text">
            <input id="inputIsClose" placeholder="Closing chat?" type="checkbox">
        </div>
    `

    const inputName = document.querySelector('#inputName')
    const inputCooldown = document.querySelector('#inputCooldown')
    const inputKeyOpen = document.querySelector('#inputOpen')
    const inputKeyInput = document.querySelector('#inputInput')
    const inputKeyClose = document.querySelector('#inputClose')
    const inputIsClose = document.querySelector('#inputIsClose')

    inputName.value = preset.name
    inputCooldown.value = preset.cooldown
    inputKeyOpen.value = preset["key-open-chat"]
    inputKeyInput.value = preset["key-input-chat"]
    inputKeyClose.value = preset["key-close-chat"].key
    inputIsClose.checked = preset["key-close-chat"]["is-close"]

    inputName.addEventListener('change', e => {
        const newName = e.target.value.trim()

        if (
            !newName ||
            newName.toLowerCase() === 'none' ||
            config.presets.some(p => p !== preset && p.name === newName)
        ) {
            e.target.value = preset.name
            return
        }

        preset.name = newName
        config["selected-preset"] = newName

        renderPresets()
        renderSelectedPreset()
    })
    inputCooldown.addEventListener('input', e => {
        preset.cooldown = Number(e.target.value)
    })
    inputKeyOpen.addEventListener('keydown', e => {
        e.preventDefault()

        preset["key-open-chat"] = e.code
        e.target.value = e.code
    })
    inputKeyInput.addEventListener('keydown', e => {
        e.preventDefault()

        preset["key-input-chat"] = e.code
        e.target.value = e.code
    })
    inputKeyClose.addEventListener('keydown', e => {
        e.preventDefault()

        preset["key-close-chat"].key = e.code
        e.target.value = e.code
    })
    inputIsClose.addEventListener('change', e => {
        preset["key-close-chat"]["is-close"] = e.target.checked
    })

    document.querySelector('.conf-open-edit-commands').innerHTML = `
        <form class="conf-open-edit-commands-create">
            <div class="conf-open-edit-commands-create-title">
                <p>Command</p>
                <p>Command key</p>
            </div>

            <div class="conf-open-edit-commands-create-line"></div>

            <div class="conf-open-edit-commands-create-inputs">
                <input placeholder="Command" required type="text" id="command">
                <input placeholder="Command key" required type="text" id="commandKey">
                <button type="submit">
                    <img draggable="false" src="./assets/img/plus.svg" alt="Create">
                </button>
            </div>
        </form>

        <div class="conf-open-edit-commands-commands-container"></div>
    `

    const form = document.querySelector('.conf-open-edit-commands-create')
    const command = document.querySelector('#command')
    const commandKey = document.querySelector('#commandKey')
    commandKey.addEventListener('keydown', e => {
        e.preventDefault()
        commandKey.value = e.code
    })
    form.addEventListener('submit', e => {
        e.preventDefault()

        const currentPreset = config.presets.find(
            p => p.name === config["selected-preset"]
        )

        if (!currentPreset) return

        currentPreset.commands.push({
            key: commandKey.value,
            command: command.value
        })

        command.value = ''
        commandKey.value = ''

        renderEditCommands()
    })

    renderEditCommands()
}



// Initialization
let config

async function init() {
    config = await readConfig()

    if (!Array.isArray(config.presets)) {
        config.presets = []
    }

    renderPresets()
    renderSelectedPreset()
}

init()



// Create preset
const createPreset = document.querySelector('.conf-create')
const inputCreatePreset = document.querySelector('.conf-create input')

createPreset.addEventListener('submit', e => {
    e.preventDefault()

    const name = inputCreatePreset.value.trim()

    if (!name || name.toLowerCase() === 'none' || config.presets.some(preset => preset.name === name)) return

    config.presets.push({
        name: name,
        cooldown: -1,
        "key-open-chat": "Enter",
        "key-input-chat": "Enter",
        "key-close-chat": {
            "is-close": false,
            key: "Enter"
        },
        commands: []
    })

    inputCreatePreset.value = ''

    renderPresets()
})



// Save config and launch program
document.querySelector(".conf-save button").addEventListener('click', async () => {
    try {
        await saveConfig(config)
        await window.electronAPI.launchExe()
    } catch (error) {
        console.error(error)
    }
})
