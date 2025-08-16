// 控制台版2048小游戏

const TILE_WIDTH = 6   // 单个方块的宽度
const TILE_HEIGHT = 3  // 单个方块的高度
const BOARD_ROWS = 4   // 行数
const BOARD_COLS = 4   // 列数
const ANIMATION_FRAME_TIME = 40  // 方块移动时每一帧的时间

// 方块的样式
const TILE_STYLES = {
    0:    { bg: '#cdc1b4', fg: '#cdc1b4' },
    2:    { bg: '#eee4da', fg: '#776e65' },
    4:    { bg: '#ede0c8', fg: '#776e65' },
    8:    { bg: '#f2b179', fg: '#f9f6f2' },
    16:   { bg: '#f59563', fg: '#f9f6f2' },
    32:   { bg: '#f67c5f', fg: '#f9f6f2' },
    64:   { bg: '#f65e3b', fg: '#f9f6f2' },
    128:  { bg: '#edcf72', fg: '#f9f6f2' },
    256:  { bg: '#edcc61', fg: '#f9f6f2' },
    512:  { bg: '#edc850', fg: '#f9f6f2' },
    1024: { bg: '#edc53f', fg: '#f9f6f2' },
    2048: { bg: '#edc22e', fg: '#f9f6f2' }
}


/** 十六进制颜色转rgb颜色
 * @param {string} 严格遵守格式的十六进制颜色字符串
 * @returns {[r: number, g: number, b: number]}
 */
function hexToRgb(hexColor) {
    return [
        parseInt(hexColor.substring(1, 3), 16),
        parseInt(hexColor.substring(3, 5), 16),
        parseInt(hexColor.substring(5, 7), 16)
    ]
}


// ANSI转义序列
class Ansi {

    // 设置控制台光标的位置 -> (x, y)
    static cursorPos = (x, y) => `\x1b[${x + 1};${y + 1}H`

    // 保存控制台光标的位置
    static saveCursorPos = "\x1b[s"

    // 恢复控制台光标的位置
    static restoreCursorPos = "\x1b[u"   

    /** 给文本添加样式
     * @param {string} text
     * @param {{fg: string, bg: string, bold: boolean}} styles
     * @returns {string}
     */
    static style(text, styles = {}) {
        const { fg, bg, bold } = styles
        let codes = []
        bold && codes.push(1)
        fg && codes.push(38, 2, ...hexToRgb(fg))
        bg && codes.push(48, 2, ...hexToRgb(bg))

        return codes.length > 0 ? `\x1b[${codes.join(';')}m${text}\x1b[0m` : text;
    }

}


class Game {
    constructor() {
        this.grid = Array(BOARD_ROWS).fill(null).map(() => Array(BOARD_COLS).fill(0))
        this.score = 0
        this.addRandomTile()
        this.addRandomTile()
    }


    // 随机添加方块
    addRandomTile() {
        const emptyTiles = [];
        for (let y = 0; y < BOARD_ROWS; y++)
            for (let x = 0; x < BOARD_COLS; x++)
                if (this.grid[y][x] == 0)
                    emptyTiles.push({ x, y })

        if (emptyTiles.length > 0) {
            const { x, y } = emptyTiles[Math.floor(Math.random() * emptyTiles.length)]
            this.grid[y][x] = Math.random() < 0.9 ? 2 : 4
        }
    }


    // 渲染一个方块
    renderTile(value) {
        const valueText = value.toString().substring(0, TILE_WIDTH - 1)
        const styles = { ...TILE_STYLES[Math.min(2048, value)], bold: true }
        const paddingLeft = Math.max(Math.floor((TILE_WIDTH - valueText.length) / 2), 0)
        const paddingRight = Math.max(TILE_WIDTH - valueText.length - paddingLeft, 0)
        const centerY = Math.floor(TILE_HEIGHT / 2)

        const tileLines = []
        for(let y = 0; y < TILE_HEIGHT; y++) {
            if(y != centerY) 
                tileLines.push(Ansi.style(' '.repeat(TILE_WIDTH), styles))
            else 
                tileLines.push(Ansi.style(`${' '.repeat(paddingLeft)}${valueText}${' '.repeat(paddingRight)}`, styles))
        }
        return tileLines
    }


    // 将整个游戏界面渲染为一整个可打印的字符串
    render() {
        let buffer = `2048 Game! Score: ${Ansi.style(this.score, {fg: '#edcf72'})}\n`
        buffer += `Use ${Ansi.style("WASD", {fg: '#edcf72'})} to Move, `
        buffer += `${Ansi.style("Q", {fg: '#edcf72'})} to Quit.\n\n`

        const boardLines = Array(BOARD_ROWS * TILE_HEIGHT).fill('');

        for (let y = 0; y < BOARD_ROWS; y++)
            for (let x = 0; x < BOARD_COLS; x++) {
                const tileLines = this.renderTile(this.grid[y][x])

                for (let i = 0; i < TILE_HEIGHT; i++)
                    boardLines[y * TILE_HEIGHT + i] += (x > 0 ? ' ': '') + tileLines[i]
            }
        
        buffer += boardLines.join('\n') + '\n'

        return buffer
    }


    isGameOver() {
        for (let y = 0; y < BOARD_ROWS; y++)
            for (let x = 0; x < BOARD_COLS; x++) {
                if (this.grid[y][x] == 0) return false
                if (y < 3 && this.grid[y][x] == this.grid[y + 1][x]) return false
                if (x < 3 && this.grid[y][x] == this.grid[y][x + 1]) return false
            }
        return true
    }


    /** 获取一行或者一列的方块的数据
     * @param {number} index 第几行或第几列
     * @param {boolean} isRow 行或列
     * @param {boolean} isForward 正向或反向
     * @returns {[{value: number, x: number, y: number, merged: false}]}
     */
    getLine(index, isRow, isForward) {
        let line = isRow ? 
            this.grid[index].map((value,x) => ({ value, x, y:index, merged: false })) : 
            this.grid.map((row,y) => ({ value: row[index], x: index, y, merged: false }))
            
        return isForward ? line : line.reverse()
    }


    /** 对一行或者一列的方块进行移动合并，返回该行方块移动时的关键帧序列
     * @param {number} lineIndex 第几行或第几列
     * @param {boolean} isRow 行或列
     * @param {boolean} isForward 正向或反向移动
     * @returns {[[{from: {x: number, y: number}, to: {x: number, y: number}}]]} 关键帧序列
     */
    moveAndMergeLine(lineIndex, isRow, isForward) { 
        const keyFrames = []
        const line = this.getLine(lineIndex, isRow, isForward)

        while(true) {
            let moved = false
            keyFrames.push([])

            for (let i = line.length - 2; i >= 0; i--) {
                if(line[i].value == 0) continue

                if(line[i+1].value == 0) {
                    line[i+1].value = line[i].value
                    line[i+1].merged = line[i].merged
                    line[i].value = 0
                    line[i].merged = false
                }

                else if((line[i+1].value == line[i].value) && (!line[i+1].merged && !line[i].merged)) {
                    line[i+1].value += line[i].value
                    line[i+1].merged = true
                    line[i].value = 0
                }

                else continue

                keyFrames[keyFrames.length - 1].push({ 
                    from: { x: line[i].x, y: line[i].y }, 
                    to: { x: line[i+1].x, y: line[i+1].y } 
                })
                moved = true
            }

            if (!moved) 
                break
        }

        keyFrames.pop()
        return keyFrames
    }


    /** 所有方块按对应方向移动和合并，并重新渲染界面
     * @param {'w' | 'a' | 's' | 'd'} direction 
     */
    move(direction) {
        const isRow = ['a', 'd'].includes(direction)
        const isForward = ['d', 's'].includes(direction)
        const linesNum = isRow ? BOARD_ROWS : BOARD_COLS

        /** @type {[[{from: {x: number, y: number}, to: {x: number, y: number}}]]} */
        let allKeyFrames = []

        // 计算所有方块移动时的关键帧序列（不移动方块）
        for (let i = 0; i < linesNum; i++) {
            const lineKeyFrames = this.moveAndMergeLine(i, isRow, isForward)

            lineKeyFrames.forEach((keyFrame, index) => {
                if (index >= allKeyFrames.length)
                    allKeyFrames.push([])
                allKeyFrames[index].push(...keyFrame)
            })
        }

        // 按照关键帧序列对方块进行移动
        for (const keyFrame of allKeyFrames) {
            for (const {from, to} of keyFrame) {
                const fromValue = this.grid[from.y][from.x]
                const toValue = this.grid[to.y][to.x]

                if(toValue == 0) {
                    this.grid[to.y][to.x] = fromValue
                    this.grid[from.y][from.x] = 0
                }

                else if(toValue == fromValue) {
                    this.grid[to.y][to.x] += fromValue
                    this.grid[from.y][from.x] = 0
                    this.score += fromValue * 2
                }
            }
            
            print(Ansi.cursorPos(0, 0))
            print(game.render())
            sleep(ANIMATION_FRAME_TIME)
        }

        if (allKeyFrames.length > 0) {
            this.addRandomTile()
            print(Ansi.cursorPos(0, 0))
            print(game.render())
        }
    }
}



const game = new Game()
print(Ansi.cursorPos(0, 0))
print(game.render())

while(true) {
    const key = waitKey()

    if(key == 'q' || key == 'Q')
        break

    else if(key == '/') {
        print(`Enter The Command (Enter "${Ansi.style('exit', {fg: '#edcf72'})}" to End):`)
        while(true) {
            print("\n> ")
            const code = input()

            if(code.trim() == "exit") {
                clear()
                print(game.render())
                break
            }

            print(eval(code))
            print(Ansi.saveCursorPos)
            print(Ansi.cursorPos(0, 0))
            print(game.render())
            print(Ansi.restoreCursorPos)
        }
    }

    else if(['W', 'A', 'S', 'D', 'w', 'a', 's', 'd'].includes(key))
        game.move(key.toLowerCase())

    if (game.isGameOver()) {
        print(Ansi.style("Game Over!\n", {fg: "#edcf72"}))
        break
    }
}