#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <windows.h>
#include <conio.h>
#include "quickjs/quickjs.h"

// 开启Windows的虚拟终端序列支持
BOOLEAN EnableVTMode() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hConsole, &dwMode))
        return FALSE;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hConsole, dwMode))
        return FALSE;

    return TRUE;
}


// 在JavaScript脚本中可以调用的print函数
JSValue JsPrint(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    if (argc < 1)
        return JS_ThrowSyntaxError(ctx, "JsPrint: Expected 1 argument, but received %d", argc);

    const char* text = JS_ToCString(ctx, argv[0]);

    if(text == NULL)
        return JS_ThrowTypeError(ctx, "JsPrint: The argument cannot be converted to string");

    printf(text);
    fflush(stdout);

    JS_FreeCString(ctx, text);

    return JS_NULL;
}

// 在JavaScript脚本中可以调用的clear函数
JSValue JsClear(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    
    DWORD dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
    COORD upperLeft = {0, 0};
    DWORD dwCharsWritten;
    
    FillConsoleOutputCharacter(hConsole, ' ', dwConSize, upperLeft, &dwCharsWritten);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, upperLeft, &dwCharsWritten);
    
    SetConsoleCursorPosition(hConsole, upperLeft);
    return JS_NULL;
}

// 在JavaScript脚本中可以调用的input函数
JSValue JsInput(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    char buffer[512];
    fgets(buffer, 512, stdin);
    return JS_NewString(ctx, buffer);
}

// 在JavaScript脚本中可以调用的waitKey函数
JSValue JsWaitKey(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    char key[2] = { 0 };
    key[0] = _getch();

    // 中断信号 (Interrupt): Ctrl + C
    if(key[0] == 0x03)
        exit(0);

    return JS_NewString(ctx, key);
}

// 在JavaScript脚本中可以调用的sleep函数
JSValue JsSleep(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    if (argc < 1)
        return JS_ThrowSyntaxError(ctx, "JsSleep: Expected 1 argument, but received %d", argc);

    unsigned int ms;
    if(JS_ToUint32(ctx, &ms, argv[0]) != 0)
        return JS_ThrowTypeError(ctx, "JsSleep: The argument cannot be converted to unsigned int");

    Sleep(ms);
    return JS_NULL;
}


// 执行JavaScript脚本文件
JSValue JS_EvalFile(JSContext* ctx, wchar_t* wszFilepath, int evalFlags) {
    FILE* file = _wfopen(wszFilepath, L"rb");
    if(file == NULL)
        return JS_ThrowSyntaxError(ctx, "Faild to open '%ls'.", wszFilepath);
        
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* jsCode = (char*)malloc(fileSize + 1);
    fread(jsCode, fileSize, 1, file);
    jsCode[fileSize] = '\0';

    fclose(file);

    char szFilepath[256];
    WideCharToMultiByte(CP_UTF8, 0, wszFilepath, -1, szFilepath, 255, NULL, NULL);

    JSValue result = JS_Eval(ctx, jsCode, fileSize, basename(szFilepath), evalFlags);

    free(jsCode);
    return result;
}


int main(int argc, char **argv) {
    // 从命令行参数中获取js文件路径，默认为 "example.js"
    wchar_t jsFilePath[MAX_PATH] = { 0 };
    MultiByteToWideChar(CP_UTF8, 0, argc > 1 ? argv[1]: "example.js", -1, jsFilePath, MAX_PATH);

    // 设置控制台编码为UTF-8
    SetConsoleOutputCP(CP_UTF8);

    // 设置控制台缓冲区大小为足够大
    setvbuf(stdout, NULL, _IOFBF, 100 * 50 * 24);

    // 启用控制台虚拟终端序列
    if(!EnableVTMode()) {
        printf("Failed to Enable the Console Virtual Terminal Sequences!\n");
        fflush(stdout);
        system("pause");
        return 0;
    }

    system("cls");

    // 创建js运行时和context
    JSRuntime *jsRuntime = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(jsRuntime);

    // 获取全局对象
    JSValue jsGlobal = JS_GetGlobalObject(ctx);

    // 将C语言函数注册到JavaScript的全局对象上
    JS_SetPropertyStr(ctx, jsGlobal, "print", JS_NewCFunction(ctx, JsPrint, "print", 1));
    JS_SetPropertyStr(ctx, jsGlobal, "input", JS_NewCFunction(ctx, JsInput, "input", 0));
    JS_SetPropertyStr(ctx, jsGlobal, "clear", JS_NewCFunction(ctx, JsClear, "clear", 0));
    JS_SetPropertyStr(ctx, jsGlobal, "waitKey", JS_NewCFunction(ctx, JsWaitKey, "waitKey", 0));
    JS_SetPropertyStr(ctx, jsGlobal, "sleep", JS_NewCFunction(ctx, JsSleep, "sleep", 1));

    // 执行 example.js
    JSValue result = JS_EvalFile(ctx, jsFilePath, JS_EVAL_TYPE_GLOBAL);

    // 检查执行结果
    if (JS_IsException(result)) {
        JSValue exception = JS_GetException(ctx);
        const char* exceptionText = JS_ToCString(ctx, exception);

        printf("[Error] %s\n", exceptionText);
        fflush(stdout);

        JS_FreeCString(ctx, exceptionText);
        JS_FreeValue(ctx, exception);
    }

    JS_FreeValue(ctx, result);
    JS_FreeContext(ctx);
    JS_FreeRuntime(jsRuntime);

    system("pause");
    return 0;
}