## 类型

### 基础类型

"无"类型: 
* `void`: 非表达式语句都是void类型，没有返回值的函数也是void类型

布尔型：
* bool: `true` / `false`

整型
* `i8`: 有符号整数
* `i16`
* `i32`
* `i64`
* `int`: 64 bits 有符号整数 
* `u8`: 无符号整数
* `u16`
* `u32`
* `u64`
* `uint`: 64 bits 无符号整数 

浮点型
* `f32`: float
* `f64`: double

字符
* `char`: utf32字符

字符串
* `string`: utf8字符串，可转为`array[u8]`

### 复杂类型

* `array[T]` 不可变数组
* `mutable_array[T]` 可变数组
* `map[K,V]` 不可变map
* `mutable_map[K,V]`可变map
* `channel[T]` 队列
* `function`

### 用户定义类型

* `class`
* `interface`

类定义：

```scala
// 接口定义
interface Bar {
    doIt(a: int, b: int): int
    doThis(a: string): string
    doThat(b: string): string
}

// 类定义
class Foo(
    val member1: int
    val member2: string
    val member3: file
): Bar

implements Foo {
public:

def doIt(a: int, b: int) = a + b
def doThis(a: string) = "hello, world: $a"
def doThat(b: string) = "do: $b"

private:
def privateDo() = "private"
} // implements Foo

```


```scala
// 对象定义
object Foo: Any {
    val member1 = 1
    val member2 = ""
    val member3 = .1
} // object Foo

implements Foo {

def doThis(a: int, b: int) = a + b * self.member1
def doThat(a: int, b: int) = a * b / self.member3

} // implements Foo
```

## 源代码结构

```
$path_of_project/  -- 项目文件夹
    src/
        dir1/
            pkg1/ -- 包: dir1.pkg1 
        dir2/
            pkg2/ -- 包: dir2.pkg2 
        pkg3/     -- 包: pkg3
        main/     -- 主包: main
            main.mai -- 主文件

$path_of_system/   -- 系统文件夹
    pkg/
        mai/      -- 系统包
            base/ -- 包: mai.base
            lang/ -- 包: mai.lang
            runtime/ -- 包: mai.runtime
        third_party/ -- 第三方包
            pkg1/ -- 包: third_party.pkg1
            pkg2/ -- 包: third_party.pkg2
```

* 项目文件夹下必须存在`src/main`才能开始执行。
* 项目的`main`包，必须存在`main`函数，才能被执行。
* `main`函数的原型为：`def main(argv: array[string])`

例子:

```scala
package main

def main(argv: array[string]) {
    println("Hello, World!")
}
```

## 语句

### 包名

每一个源文件必须写明自己属于哪个包

```
package_stmt := `package' identifier
```

```scala
package main // main包
```

### 引用包

```
import_stmt := `import' [ alias ] identifier { `.' identifier }
import_stmt := `import' `{' { [ alias ] identifier { `.' identifier } } `}'
alias := `*' | identifier
```

引用一个包：

```scala
import json
import foo.bar
```

引用多个包：

```scala
import {
    json
    foo.bar
    * mai.lang
    * mai.runtime
}
```


### 变量声明

变量声明语句为：

```
variable_declaration := <`val'|`var'> identifier [`:' type [= initializer]]
constant_declaration := `const' identifier [`:' type] = initializer
```

值类型 `val`：表示值不能改变
```scala
val i = 1
val b = i + 1
val c = "100"
val c: Foo = Foo(1, 2, 3)
```

变量 `var`: 表示值可以改变

```scala
var i = 1
i++
var b + i + 1
b = i++
var c = Foo(1, 2, 3)
c.a = 1
```

### 函数声明/定义

```
function_declaration := `native' `def' identifier function_prototype
function_definition := `def' identifier function_prototype function_body
function_prototype := `(' parameters `)' [ `:' type ]
function_body := `=' expr
| `{' block `}'
parameters := [ expr { `,' expr } ] [ `...' ]

instance_method_implements := `implements' identifier `{' { method_definition } `}'

class_method_definitions := `defines' identifier `{' { method_definition } `}'

method_definition := `public' `:'
| `protected' `:'
| `private' `:'
| function_definition

```

> 可以用`native`修饰，声明由C++实现的函数。

```scala
implements outside.Foo {
public:

def doIt(a: int, b: int) { 
    println(a + b + self.c)
}

def doThis(a: string) = "Hello, $a"

def doVargs(a: string, ...) {
    var args = mutable_array[any]()
    for (_, arg in ...) {
        args += (.. -> arg)
    }
    doVargsAgain(a, ...) // 可以继续传给另一个可变参数函数
    doVargsAgain(a, args...) // 或者这样
}

def doVargsAgain(a: string, ...) {
    // TODO:
}


native def doAny(a: any)

private:

def doThat(a: string) {
    println("private method")
}

}
```

### 数组/Map

```
array_initializer := <`array' | `mutable_array'> [ `[' type `]' ] `(' { expr {, expr } } `)'
map_initializer := < `map' | `mutable_map'> [ `[' type `,' type `]' ] `(' { pair {, pair } } `)'

collection_addition := expr `+=' `(' < pair | appendation_pair > `)'
| expr `+' `(' < pair | appendation_pair > `)'

collection_deletion := expr `-=' expr
| expr `-' expr

pair := expr `->' expr
appendation_pair := `..' `->' expr
```

> 数组和Map都分为不可变/可变两种

```scala

// 不可变类型
var a = array(1, 2, 3, 4, 5)
var m = map("ok" -> 1, "name" -> 2, "age" -> 3)
a[0] = -1 // 编译错误：a不可变
a[1] = -2 // 编译错误：a不可变

a += (.. -> 6) // append a
a += (0 -> -1) // set a[0] = -1 并新生成一个array
a -= 0 // delete a[0] 并新生成一个array

m["ok"] = -1 // 编译错误：a不可变
m["oops"] = 4 // 编译错误：a不可变

m += ("ok" -> -1) // put m["ok"] = -1 并新生成一个map 
m += ("oops" -> 4) // put m["oops"] = 4 并新生成一个map
m -= "oops" // delete m["oops"] 并新生成一个map


// 可变类型
val a = mutable_array(1, 2, 3, 4)
val m = mutable_map("ok" -> 1, "name" -> 2, "age" -> 3)
a[0] = -1
a[1] = -2
val b = a + (.. -> 5) // 不一定会生成新array
m["ok"] = -1
m["oops"] = 5
```

### 接口/类/对象定义

```
interface_definition := `interface' identifier `{' interface_block `}'
interface_block := interface_method { interface_method }
interface_method := identifier function_prototype

class_definition := `class' [ constructor ] [ extends_cause ] `{' class_block `}'
object_definition := `object' [ extends_cause ] `{' class_block `}'

constructor := `(' { constructor_parameter_declaraion } `)'
constructor_parameter_declaraion := `val' identifier `:' type `,'
| `public' `:'
| `protected' `:'
| `private' `:'

extends_cause := `:' type [ `(' expr { `,' expr } `)' ]

class_block := { class_member_declaration }
class_member_declaration := variable_declaraion
| `public' `:'
| `protected' `:'
| `private' `:'
```

> 接口仅仅是规范，在实现中没有任何对应实体
> 基于类似Objective-C的消息发送机制+ducking type，只要实现了某接口的方法，他就是某接口。


```scala

// 接口定义
interface Baz {
    doIt(a: int, b: int): int
    doThis(a: string)
    doThat(a: string): f32
}

// 类定义
class Foo(
private:
    val i: int,
    val j: int,
    val k: f32,
    l: int,
    m: int
): FooBase(l, m) { // 继承自FooBase
    var name: string
}

implements Foo {

def doIt() = self.i + self.j + self.k

} // implements Foo

val foo = Foo(1, 2, 3)
foo.name = "foo"
println(foo.i, foo.j, foo.k) // error, i, j, k是私有成员
println(foo.doIt())


// 对象定义
object Bar {
    val i = 100
    val j = .1
    var name: string
}

defines Bar {

def doIt(a: int, b: int) = a + b + Bar.i
def doThis(a: string) = println("hello, ${Bar.name}, $a")
doThat(a: string): f32 = Bar.j

}

println(Bar.doIt(1, 2, 3))
println(Bar.doThis("haha"))
println(Bar.doThat(""))

```

### 循环语句

```
while_loop := `while' `(' [ variable_declaration `;' ] condition `)' `{' block `}'

for_loop := `for' `(' identifier `in' expr `..' expr `)' `{' block `}'
| `for' `(' identifer [ `,' identifer ] in `)' `{' block `}'
```

while 循环

```scala
while (true) {
    // 死循环
}

while (val ok = check; ok) {
    // 当ok==true时循环
}

for (i in 0..100) {
    // 循环100次，i的范围[0, 100)
}

val m = map("first" -> 1, "second" -> 2, "third" -> 3)
for (k, v in m) {
    // 遍历m，k为key，v为value m[k] == v
}

val a = array(1, 2, 3, 4, 5)
for (i, v in a) {
    // 遍历a，i为下标，v为值 a[i] == v
}
```

### 协程语句

协程间通信：方案1：类似go的channel通信方案

```
`run' calling
`send' expr `<-' expr
`receive' expr `->' identifer
```

```scala
// 创建一个队列长度为1的channel
val ch = channel[int](1)
// 启动协程，入口函数为foo
run foo(ch)

for (i in 0..100) {
    val ok = send ch <- i // 发送数据到ch
    if (!ok) {
        throw Exception("fail")
    }
}
close(ch)

def foo(ch: channel[int]) {
    while (true) {
        var n: int
        val ok = receive ch -> n // 从ch接收数据
        if (!ok) { // is close?
            break
        }
        println("received $n")
    }
}
```

协程间通信：方案2：类似erlang的消息发送方案通信方案

```
spwan_expr := `spwan' calling
fork_expr := `fork' calling
send_expr := `send' expr `<-' expr
receive_stmt := `receive' `(' identifer `:' `channel' `)' `{' match_block `}'

match_block := { match_case `->' stmt }

match_case := identifer `:' type
| `(' identifer `:' type `,' identifer `)'
| `else'
| `done'

```

```scala
val pid = spwan foo(1, 2)
send pid <- 100
processReceive(pid)
send pid <- "foo"
processReceive(pid)
send pid <- .1
processReceive(pid)
sendDone(pid)

def processReceive(pid: Pid) {
    receive(from: Pid) {
        (n: int, pid) -> println("result: $n")
        (s: string, pid) -> println("result: $s")
        (a: any, pid) -> println("result: $a")
        else -> println("result: unknown") // 消耗未知消息，防止堆积
        done -> println("done!")
    }
}


def foo(a: int, b: int) {
    while (true) {
        receive(from: Pid) {
            n: int -> {
                println("message: $n")
                send from <- n + 1 + a + b
            }
            s: string -> {
                println("message $s")
                send from <- "hello, $s"
            }
            a: any -> {
                println("unknown message $a")
                send from <- nil
            }
            done -> return
        }
    }
}

```

### 表达式

表达式有很多类型

#### 基本表达式

```
simple_expr := integral_literal
| floating_literal
| boolean_literal
| nil_literal
| string_literal
| lambda_literal
| array_initliazer
| map_initliazer
| identifer { `.' identifer }
| calling
| accessor

integral_literal := [ `-' | `+' ] digit { digit } [ integral_suffix ]
| [ `-' | `+' ] `0x' hex_digit { hex_digit } [ integral_suffix ]
| [ `-' | `+' ] `0' oct_digit { oct_digit } [ integral_suffix ]

floating_literal := [ `-' | `+' ] { digit } `.' digit { digit } [ `e' digit { digit } ]

boolean_literal := `true' | `false'

nil_literal := `nil'

identifer := <alpha | digit | `_' | `$'> { alpha | digit | `_' }

accessor := expr `[' expr `]'

integral_suffix := `i8' | `i16' | `i32' | `i64' | `u8' | `u16' | `u32' | `u64' | `u'

floating_suffix := `f' | `d'

alpha := [a-zA-Z]
hex_digit := [0-9a-fA-F]
oct_digit := [0-7]
digit := [0-9]
```

```scala
// 整数字面值
0xffu8 // u8: 0xff
0xffi8 // i8: 0xff
0u // uint: 0
1 // int: 1
0777u16 // u16: 0777

// 浮点数字面值
.0 // f32: 0.0
.0d // f64: 0.0
1.1 // f32: 1.1
1.1f // f32: 1.1
1.1e7 // f32: 11000000.0

// lambda
val foo = { a: int, b: int -> a + b }
foo(1, 2) // 3

// 数组初始化
[1, 2, 3, 4] // array[int]
["1", "2", "3"] // array[string]
[1, .1, "1"] // array[any]

// map初始化
map{"key": 1, "value": 2} // map[string, int]
map{"key1": 1, "key2": .1} // map[string, any]
map{1: "val1", .1: "val2"} // error

```

#### 字符串字面值

```
string_literal := `"' { utf8_char | template_replacement | escape_pattern } `"'
| ``' { utf8_char } ``'

template_replacement := `$' identifier
| `$' `{' expr `}'
escape_pattern := `\r'
| `\n'
| `\t'
| `\x' hex_digit hex_digit
| `\u' digit digit digit digit
utf8_char := ...
```

> 支持`$identifier`和`${expr}`形式的字符串模板拼接

```scala
"Hello, World\n" // 普通字符串
`"Hello", World` // 不转义的字符串
"Hello, $word" // 字符串模板
"Hello, ${word.say()}" // 字符串模板
```

#### 表达式列表

```
expr_list := expr { `,' expr }
```

> 用逗号(`,`)分隔的表达式，取最后一个表达式的值

```scala
@Inline("force")
def foo() =
    val a = 1,
    val b = doThat(),
    val c = a + b * doThis(),
    c + doIt();
```


#### 算术表达式

用于算术运算的表达式是，所有数值类型都可使用：

```
binary_expr := expr <`+' | `-' | `*' | `/' | `%'> expr

unary_expr := <`+'|`-'> expr

increment_expr := expr <`--' | `++'>
| <`--' | `++'> expr

expr := `(' expr ')'
```

```scala
val a = 1, b = 2
val c = (a + b) * b - 100

var a = 1
a++
a--
--a
++a

val a = 1
a++ // error

val a = .1, b = .1231
val c = (a + b) * a - 100.0
```

#### 位运算表达式

位运算支持整数类型：

```
bitwise_expr := expr <`&' | `|' | `^' | `<<' | `>>'> expr
| `~' expr
```

```scala
val a = 0x111111, b = 0xff
val c = (a & b) << 1u
```

#### 布尔表达式

```
condition := expr <`<' | `<=' | `>' | `>=' |`==' | `!='> expr
| expr `&&' expr
| expr `||' expr
| `!' expr
```

#### 条件表达式

```
when_expr := `when' condition `then' expr [`else' expr]

if_expr := if_cause stmt { `else' if_cause stmt } [ `else' stmt ]
if_cause := `if' `(' [ variable_declaration `;' ] condition `)'

match_expr := match `(' expr `)' `{' match_block `}'
match_block := match_case { match_case } [ match_else ]
match_case := identifier `:' type `->' stmt
| <integral_literal | floating_literal | string_literal> `->' stmt
| condition `->' stmt
match_else := `else' `->' stmt

```

```scala
// 当 a > 100 时，将c初始化为-1，否则为999
var c = when a > 100 then -1 else 999
// 当 a < 100 时，将c赋值为2，否则不做任何操作
c = when a < 100 then 2

if (val ok = check(); ok) {
    // ok作用域只在这个block里
}

if (a >= 0 && a < 100) {
    println("step 1")
} else if (a >= 100 && a < 200) {
    println("step 2")
} else {
    println("step 3")
}

match (a) {
    n: int -> println("is int")
    1.1 -> println("is 1.1")
    a > b -> println("a > b")
    else -> println("otherwise: $a")
}
```


## 实现

### 内存块(span)

* * *

> 为了实现GC标记存活对象，使用`span`组织内存，堆对象和非堆对象放入不同的`span`中，然后使用bitmap标记哪些`span`是存储的是堆对象的引用，哪些存储的是数据，即不需要GC扫描的部分

```
- Span16 最小的span，用于栈中存储局部变量
-- 2 * ptrs / 2 * u64
-- 4 * u32
-- 8 * u16
-- 16 * u8

|<------ 16 bytes ----->|
+-----------+-----------+
|   ptr[1]  |   ptr[0]  |
+-----------+-----------+
|    u64    |    u64    |
+-----+-----+-----+-----+
| u32 | u32 | u32 | u32 |
+-----+-----+-----+-----+
|  3  |  2  |  1  |  0  |

- Span32 中型span，用于常量池
-- 4 * ptrs / 4 * u64
-- 8 * u32
-- 16 * u16
-- 32 * u8

|<------------------ 32 bytes ----------------->|
+-----------+-----------+-----------+-----------+
|   ptr[3]  |   ptr[2]  |   ptr[1]  |   ptr[0]  |
+-----------+-----------+-----------+-----------+
|    u64    |    u64    |    u64    |    u64    |
+-----+-----+-----+-----+-----+-----+-----+-----+
| u32 | u32 | u32 | u32 | u32 | u32 | u32 | u32 |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |

- Span64 最大的span，用于全局变量
-- 8 * ptrs / 8 * u64
-- 16 * u32
-- 32 * u16
-- 64 * u8

|<------------------------------ 64 bytes ----------------------------->|
+--------+--------+--------+--------+--------+--------+--------+--------+
| ptr[7] | ptr[6] | ptr[5] | ptr[4] | ptr[3] | ptr[2] | ptr[1] | ptr[0] |
+--------+--------+--------+--------+--------+--------+--------+--------+
|   u64  |   u64  |   u64  |   u64  |   u64  |   u64  |   u64  |   u64  |
+--------+--------+--------+--------+--------+--------+--------+--------+
```

### 内存管理

* * *

存储区域分为以下部分：

* 静态全局变量存储区: 使用`Span64`存储，用bitmap标记出引用区域
* 栈：函数调用的活动空间
* 堆
    * `New Space`: 年轻代
    * `Old Space`: 老年代
    * `Large Space`: 大内存区，也是老年代
* 堆外内存
    * `Metadata Space`: 元数据区，使用`Span64`存储以下内容
        * 类型信息
        * 常量
        * backtrace和调试用的源代码信息
    * `Code Space`: 代码区，存储可执行代码


```C++
union SpanPart64 {
    uint64_t u64;
    int64_t  i64;
    double   f64;
}

union SpanPart32 {
    uint32_t u32;
    int32_t  i32;
    float    f32;
}

union SpanPartPtr {
    void *pv;
    uint8_t *addr;
    Any *any;
}

#define DECLARE_SPAN_PARTS(n) \
    SpanPartPtr ptr[n]; \
    SpanPart64 v64[n]; \
    SpanPart32 v32[n*2]; \
    uint16_t u16[n*4]; \
    int16_t i16[n*4]; \
    int8_t i8[n*8]; \
    uint8_t u8[n*8]
    
union Span16 {
    DECLARE_SPAN_PARTS(2);
}

union Span32 {
    DECLARE_SPAN_PARTS(4);
}

union Span64 {
    DECLARE_SPAN_PARTS(8);
}

static_assert(sizeof(Span16) == 16, "");

```

### 栈

* * *

> 每个`routine`有一个自己的栈，参数全部通过栈传递，返回值通过寄存器传递(x64为`rax`)

* 调用者栈(`caller`)

```
字节码函数的调用栈

+-------------------+
| return address    | 8 bytes
+-------------------+
| saved sp          | 8 bytes
+-------------------+ --------
| maker             | 4 bytes
+-------------------+ --------
| pc                | 4 bytes
+-------------------+
| callee function   | 8 bytes
+-------------------+
| constants pool    | 8 bytes
+-------------------+
| local var span[0] | local variable spans
+-------------------+
| local var span[0] | N * Span16
+-------------------+
| ... ... ... ...   |
+-------------------+ --------
| callee function   | 8 bytes
+-------------------+ 
| argv[1]           | first argument
+-------------------+ 
| argv[2]           | secrond argument
+-------------------+ 
| ... ... ... ...   | ...others arguments
+-------------------+ 
| vargs             | optional: 8 bytes
+-------------------+ 
```
> sp = stack pointer 栈指针，在x64下就是`rpb`

* 被调用者栈(`callee`)

```
+-------------------+ -------- caller stack --------
| argv[1]           | first argument
+-------------------+ 
| argv[2]           | secrond argument
+-------------------+ 
| ... ... ... ...   | ...others arguments
+-------------------+ 
| vargs             | optional: 8 bytes
+-------------------+ --------- callee stack -------
| return address    | 8 bytes
+-------------------+
| saved sp          | 8 bytes
+-------------------+ --------
| maker             | 4 bytes
+-------------------+
| pc                | 4 bytes
+-------------------+
| callee function   | 8 bytes
+-------------------+
| constants pool    | 8 bytes
+-------------------+ --------
| local var span[0] | local variable spans
+-------------------+
| local var span[1] | N * Span16
+-------------------+
| ... ... ... ...   |
+-------------------+ --------
```

### Isolate和Context

* * *

> `Isolate`就是对虚拟机的抽象

```
- 对虚拟机的抽象
+=============================+
[           Isolate           ]
+=============================+
|                 heap: Heap* | Heap memory management
|-----------------------------|
|         metadata: Metadata* | Metadata space
|-----------------------------|
|  global_space: GlobalSpace* | global variables space
|-----------------------------|
|      all_context: Context[] | All of contexts
|-----------------------------|
|      scheduler: Scheduler * | machines and coroutines management
|-----------------------------|
|          machine0: Machine* | The main os thread
+-----------------------------+

```

使用`Isolate`的例子

```C++
int main(int argc, char *argv[]) {
    mai::Isolate *isolate = mai::Isolate::New(options);
    mai::Isolate::Scope isolate_scope(isolate);
    if (auto err = isolate->Initialize(); err.fail()) {
        // handler errors
        return -1
    }

    mai::Context *context = Context::Get();
    if (auto err = context->Compile(/*被编译的代码*/); err.fail()) {
        // handler errors
    }
    context->NewArray();
    context->LetString("hello");
    context->LetString("world");
    context->LetString("final");
    Mcontext->Append(3)
    context->SetArgv();

    context->LetFunction("main.main");
    context->Run();
    return 0;
}
```

### Machine和Coroutine

* * *

> Machine是对操作系统线程的抽象。
> Coroutine是对协程的抽象。

```
- 调度器，管理调度Coroutine

+=============================+
[           Scheduler         ]
+=============================+
| free_coroutines: Coroutine* | global free coroutines(coroutine pool)
|-----------------------------|
|     all_machines: Machine[] | all of machines(all of os threads)
|-----------------------------|
| all_coroutines: Coroutine[] | all of coroutines
|-----------------------------|
|      stack_pool: StackPool* | function stack pool
|-----------------------------|
|         machine_tls: TLSKey | current machine TLS
|-----------------------------|
|       coroutine_tls: TLSKey | current coroutine TLS
|-----------------------------|
|            unqiue_coid: u64 | coroutine id counter

- 对操作系统线程的抽象
-- 线程的状态:

Machine::kDead: 线程结束，无法再执行协程
Machine::kIdle: 空闲，没有协程供运行
Machine::kRunning: 正在运行中

-- 抢占协程：空闲的线程可以抢占别的线程拥有的协程

+============================+
[           Machine          ]
+============================+
|                   tid: u32 | thread id
|----------------------------|
|               state: State | machine current state
|----------------------------|
| free_coroutines: Coroutine* | local free coroutines(coroutine pool)
|----------------------------|
|      runnable: Coroutine[] | waiting for running coroutines
|----------------------------|
|        running: Coroutine* | current running coroutine
|----------------------------|
|             exclusion: u64 | exclusion counter if > 0 can not be preempted

- 对协程的抽象
-- 协程的状态:

Coroutine::kDead: 协程死亡，协程入口函数执行完后处于此状态
Coroutine::kIdle: 空闲，未被任何一个machine调度
Coroutine::kRunning: 正在运行中
Coroutine::kFallIn: 在运行中，但是在调用一个Runtime的C++函数

+============================+
[          Coroutine         ]
+============================+
|                  coid: u64 | coroutine id
|----------------------------|
|               state: State | coroutine current state
|----------------------------|
|      entry_point: Closure* | entry function
|----------------------------|
|            owner: Machine* | owner machine
|----------------------------|
|        caught: CaughtNode* | exception hook for exception caught
|----------------------------|
|                bp: Address | saved frame pointer
|----------------------------|
|                sp: Address | saved stack pointer
|----------------------------|
|                pc: Address | program counter
|----------------------------|
|                   acc: u64 | tmp saved ACC
|----------------------------|
|                  xacc: f64 | tmp saved XACC
|----------------------------|
|              stack: Stack* | function stack

```

```asm
SCRATCH = r10

call Coroutine::Current ; get coroutine tls
movq rbx, rax ; backup
movq SCRATCH, Coroutine_caught(rax) ; SCRATCH = coroutine.caught
movq caught_point(rbp), SCRATCH ; caught.next = coroutine.caught
leaq rax, caught_point(rbp)
movq Coroutine_caught(rbx), rax ; coroutine.caught = &caught

; save stack env to caught_node
movq SCRATCH, Coroutine_caught(rbx)
movq CaughtNode_bp(SCRATCH), rbp
movq CaughtNode_sp(SCRATCH), rsp
movq rax, @exception_handler
movq CaughtNode_sp(SCRATCH), rax
jmp near @done
nop
... ; byte patches
nop
@exception_handler:
...
@done:
...


movq rax, Coroutine_caught(r15)
movq rax, CaughtNode_pc(rax)
jmp far rax
```

```C++
struct CaughtNode {
    CaughtNode *next;
    Address pc;
    Address bp;
    Address sp;
}

// 异常对象，本身存放在rax中
// 异常处理模仿jvm的实现，使用异常表。
```

> Machine和Coroutine都要设置TLS，已加便快速访问。

`Machine`(线程)的创建:
* 必须至少有一个
* `machine0`表示主线程，即启动虚拟机的线程。
* 线程数由传入的参数或者cpu核数决定。
* 线程数启动后不改变。

`Coroutine`的创建：
* 函数只能在`Coroutine`里执行。
* 每个`Context`会创建一个`Coroutine`用来执行入口函数`main.main()`

### 函数

函数对象称为: `Closure`
函数的元数据称为: `Function`

函数元数据`Function`包含：
* 函数名
* 常量池
* backtrace和debug用的源代码信息
* 字节码或者原生代码
* 捕获外部变量的描述表
* 异常表

```
+=========================================+
[                 Closure                 ]
+=========================================+
|                               tags: u32 | is cxx fn or compiled fn?
|----------------------+-------------------|
| cxx_fn: CxxFunction* | mai_fn :Function* | function proto or cxx function
|----------------------+-------------------|
|                  captured_var_size: u32 |
|-----------------------------------------|
|             captured_var: CapturedVar[] | captured variables
+-----------------------------------------+


+=========================================+
[                 Function                ]
+=========================================+
|                             name: char* | function full name
|-----------------------------------------|
|                       proto_desc: char* | function prototye description
|-----------------------------------------|
|                         stack_size: u32 | stack size for invoking frame
|-----------------------------------------|
|                     stack_bitmap: u32[] | stack spans bitmap for GC
|-----------------------------------------|
|               const_pool: ConstantPool* | constants pool
|-----------------------------------------|
|            source_line: SourceLineInfo* | source info for debug and backtrace
|-------------+---------------------------|
| code: Code* | bytecodes: BytecodeArray* | bytecodes or native code
|-------------+---------------------------|
|                  captured_var_size: u32 |
|-----------------------------------------|
|        captured_vars: CpaturedVarDesc[] | capture variable for closure
|-----------------------------------------|
|               exception_table_size: u32 |
|-----------------------------------------|
| exception_table: ExceptionHandlerDesc[] | exception table
```

### 对象/类型

* * *

```
Type:
    id: TypeId(u64)
    name: String *
    filed_size: i32
    fields: Field *
    fields_lookup: HashMap *
    
    
Class(Type):
    method_size: i32
    methods: Method *
    methods_lookup: HashMap *

Field: 
    name: String *
    type: Type *
    index: u32
    offset: u32
    
// Map
Map(Any):
    ...
```

#### Any结构

> Any是所有值类型的基类
> 其中包含有类型信息、tags

```
+===================+
|         Any       |
+===================+
|      type: Type * |
+-------------------+
|         tags: u32 |
+-------------------+

|<-----Any Header----->|
+----------+-----------+------------------
| type:ptr | tags: u32 | value's data ...
+----------+-----------+------------------

type:ptr的结构:

|<------unused bits----->|<--------------valid address----------------->|
|   7    |   6    |   5  | |   4    |   3    |   2    |   1    |   0    |
+--------+--------+--------+--------+--------+--------+--------+--------+
|uuuuuuuu|uuuuuuuu|uuuuuuvv|vvvvvvvv|vvvvvvvv|vvvvvvvv|vvvvvvvv|vvvvvv00|
+--------+--------+--------+--------+--------+--------+--------+--------+

[0~41bit] 是指针有效地址占用42 bits: 用vvvv表示
[42~63bit] 未使用22 bits：用uuuu表示
[0~1bit] 是指针低地址，因为分配内存总对齐到四字节，因此低2 bits恒为0，用来存储maker

maker == 1: 表示此对象被移动，type:ptr保存forward地址。
maker == 0: 表示此队形未移动，type:ptr指向类型对象

```

> 每个堆对象的头部都包含`Any Header`


#### 数组

```
-- 数组 Header
+===================+
[       Array       ]
+===================+
|     Any Header    |
|-------------------|
|          len: u32 |
|-------------------|
|          cap: u32 |
+-------------------+

- 基础类型的特化数组
-- 支持 bool i8 u8
+===================+
[   Array8(Array)   ]
+===================+
|   Array Header    |
|-------------------|
|       elems: u8[] |
+-------------------+

-- 支持 i16 u16
+===================+
[   Array16(Array)  ]
+===================+
|   Array Header    |
|-------------------|
|      elems: u16[] |
+-------------------+

-- 支持 i32 u32 f32
+===================+
[   Array32(Array)  ]
+===================+
|   Array Header    |
|-------------------|
|      elems: u32[] |
+-------------------+

-- 支持 int uint i64 u64 f64
+===================+
[   Array64(Array)  ]
+===================+
|   Array Header    |
|-------------------|
|      elems: u64[] |
+-------------------+

-- 其他所有类型
+===================+
[GenericArray(Array)]
+===================+
|   Array Header    |
|-------------------|
|     elems: Any*[] |
+-------------------+
```

数组的内存结构为：

```
             |<--Array Header--->|<------Elements----------------------------->|
+------------+---------+---------+---------+---------+---------+     +---------+
| Any Header | len:u32 | cap:u32 | elem[0] | elem[1] | elem[2] | ... | elem[N] |
+------------+---------+---------+---------+---------+---------+     +---------+
```

### 字节码

* * *

字节码为定长`u32`编码，主要分为`N`, `A`, `AB`, `ABC`三类：

```
- N型：没有参数

|< 8b >|<----------------- 24 bits ---------------->|
+------+--------------------------------------------+
| code |                    Unused                  |
+------+--------------------------------------------+

- A型：有1个参数

|< 8b >|<----------------- 24 bits ---------------->|
+------+--------------------------------------------+
| code |                Parameter A                 |
+------+--------------------------------------------+

- AB型：有2个参数

|< 8b >|<----- 12 bits ----->|<------ 12 bits ----->|
+------+---------------------+----------------------+
| code |     Parameter A     |      Parameter B     |
+------+---------------------+----------------------+

- ABC型：有3个参数

|< 8b >|<-- 8 bits -->|<-- 8 bits -->|<-- 8 bits -->|
+------+--------------+--------------+--------------+
| code | Parameter A  | Parameter B  | Parameter C  |
+------+--------------+--------------+--------------+
```

#### 命令代码


| Code                | 编码类型 | 作用 | 参数A                       | 参数B          | 参数C |
| ------------------- | -------: | ---- | --------------------------- | -------------- | ----- |
| `Ldar32`            |      `A` |      | `i24` 栈偏移量              |                |       |
| `Ldar64`            |      `A` |      | `i24` 栈偏移量              |                |       |
| `Ldaf32`            |      `A` |      | `i24` 栈偏移量              |                |       |
| `Ldaf64`            |      `A` |      | `i24` 栈偏移量              |                |       |
| `LdaZero`           |      `N` |      |                             |                |       |
| `LdaSmi32`          |      `A` |      |                             |                |       |
| `LdaTrue`           |      `N` |      |                             |                |       |
| `LdaFalse`          |      `N` |      |                             |                |       |
| `Ldak32`            |      `A` |      | `u24` `const_pool` 偏移量   |                |       |
| `Ldak64`            |      `A` |      | `u24` `const_pool` 偏移量   |                |       |
| `Ldakf32`           |      `A` |      | `u24` `const_pool` 偏移量   |                |       |
| `Ldakf64`           |      `A` |      | `u24` `const_pool` 偏移量   |                |       |
| `LdaGlobal32`       |      `A` |      | `u24` `global_space` 偏移量 |                |       |
| `LdaGlobal64`       |      `A` |      | `u24` `global_space` 偏移量 |                |       |
| `LdaGlobalf32`      |      `A` |      | `u24` `global_space` 偏移量 |                |       |
| `LdaGlobalf64`      |      `A` |      | `u24` `global_space` 偏移量 |                |       |
| `Star32`            |      `A` |      | `u24` 栈偏移量              |                |       |
| `Star64`            |      `A` |      | `u24` 栈偏移量              |                |       |
| `Staf32`            |      `A` |      | `u24` 栈偏移量              |                |       |
| `Staf64`            |      `A` |      | `u24` 栈偏移量              |                |       |
| `Move32`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Move64`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Add32`             |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Add64`             |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Addf32`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Addf64`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Sub32`             |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Sub64`             |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Subf32`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Subf64`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Mul32`             |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Mul64`             |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Mulf32`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Mulf64`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `IMul32`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `IMul64`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Div32`             |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Div64`             |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Divf32`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Divf64`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `IDiv32`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `IDiv64`            |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Mod32`             |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `Mod64`             |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `I32toI8`           |      `A` |      | `u24` 栈偏移量              |                |       |
| `U32toI8`           |      `A` |      | `u24` 栈偏移量              |                |       |
| `I32toU8`           |      `A` |      | `u24` 栈偏移量              |                |       |
| `U32toU8`           |      `A` |      | `u24` 栈偏移量              |                |       |
| `I32toI16`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `U32toU16`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `I64toI32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `U64toI32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `F32toI32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `F64toI32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `I8toU32`           |      `A` |      | `u24` 栈偏移量              |                |       |
| `U8toU32`           |      `A` |      | `u24` 栈偏移量              |                |       |
| `I16toU32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `U16toU32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `I64toU32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `U64toU32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `F32toU32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `F64toU32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `I32toI64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `U32toI64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `F32toI64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `F64toI64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `I32toU64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `U32toU64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `F32toU64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `F64toU64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `I32toF32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `U32toF32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `I64toF32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `U64toF32`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `I32toF64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `U32toF64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `I64toF64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `U64toF64`          |      `A` |      | `u24` 栈偏移量              |                |       |
| `BitwiseOr32`       |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `BitwiseOr64`       |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `BitwiseAnd32`      |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `BitwiseAnd64`      |     `AB` |      | `u12` 栈偏移量              | `u12` 栈偏移量 |       |
| `BitwiseNot32`      |      `A` |      | `u12` 栈偏移量              |                |       |
| `BitwiseNot64`      |      `A` |      | `u12` 栈偏移量              |                |       |
| `BitwiseShl32`      |      `A` |      | `u12` 栈偏移量              |                |       |
| `BitwiseShl64`      |      `A` |      | `u12` 栈偏移量              |                |       |
| `BitwiseShr32`      |      `A` |      | `u12` 栈偏移量              |                |       |
| `BitwiseShr64`      |      `A` |      | `u12` 栈偏移量              |                |       |
| `BitwiseLogicShr32` |      `A` |      | `u12` 栈偏移量              |                |       |
| `BitwiseLogicShr64` |      `A` |      | `u12` 栈偏移量              |                |       |
| `Throw`             |      `N` |      |                             |                |       |

类型转换：任意两类型间，最多只需要两步转换：
规则：
1. 非`i32`,`u32`类型，先按有无符号，转换到`i32`，`u32`。
2. 再从`i32`,`u32`类型转换到目标类型。

* From `i8`, To:
    * `i8` -> None
    * `u8` -> `I8ToU32` + `U32ToU8`
    * `i16` -> `I8ToI32` + `I32ToI16`
    * `u16` -> `I8ToU32` + `U32ToU16`
    * `i32` -> `I8ToI32`
    * `u32` -> `I8ToU32`
    * `i64` -> `I8ToI32` + `I32ToI64`
    * `u64` -> `I8ToU32` + `I32ToU64`
    * `f32` -> `I8ToI32` + `I32ToF32`
    * `f64` -> `I8ToI32` + `I32ToF64`
* From `u8`, To:
    * `i8` -> `U8ToU32` + `U32ToI8`
    * `u8` -> None
    * `i16` -> `U8ToI32` + `I32ToI16`
    * `u16` -> `U8ToU32` + `U32ToU16`
    * `i32` -> `U8ToI32`
    * `u32` -> `U8ToU32`
    * `i64` -> `U8ToI32` + `I32ToI64`
    * `u64` -> `U8ToU32` + `I32ToU64`
    * `f32` -> `U8ToI32` + `I32ToF32`
    * `f64` -> `U8ToI32` + `I32ToF64`
* 其他类型以此类推...

#### 字节码执行

字节码执行环境：

```
SCRATCH = r12
BC = r13
BC_ARRAY = r14
CO = r15
ACC = rax
XACC = xmm0
Argv_0...Argv_8 = rdi, rsi ...
```

* `Trampoline`: 字节码执行的入口，负责从外部环境进入字节码执行环境。

```asm
; Prototype: void Trampoline(Coroutine *co)
; Save registers
; TODO:

; Save native stack and frame
call Coroutine::Current() ; rax = current_coroutine
movq CO, rax
movq Coroutine_bp(CO), rbp
movq Coroutine_sp(CO), rsp
leaq rax, 0(rip)
movq Coroutine_pc(CO), rax

; Set root exception handler
movq rbx, rbp
subq rbx, stack_frame_caught_point
movq Coroutine_caught(CO), rbx ; coroutine.caught = &caught
movq CaughtNode_next(rbx), 0 ; caught.next = nullptr
movq CaughtNode_bp(rbx), rbp ; caught.bp = system rbp
movq CaughtNode_sp(rbx), rsp ; caught.sp = system rsp
movq rax, @uncaught_handler
movq CaughtNode_pc(rbx), rax ; caught.pc = @exception_handler
jmp near @entry
nop
nop
... ; byte patches
nop

; Handler root exception
@uncaught_handler: ; rax = top exception object
; switch to system stack
movq r11, Coroutine::Uncaught
movq Argv_0, CO
movq Argv_1, rax
call SwitchSystemStackCall; call co->Uncaught(exception)
jmp near @done

; function entry
; SCRATCH = coroutine
@entry:
movq rbp, Coroutine_bp(CO)
movq rsp, Coroutine_sp(CO)
cmpq Coroutine_reentrant(CO), 0
jg @resume ; if (coroutine->reentrant > 0) 
; first calling
incl Coroutine_reentrant(CO) ; coroutine.reentrant++
movq Argv_0, Coroutine_entry_point(CO)
call InterpreterPump
jmp far @done

; coroutine->reentrant > 0, means: should resume this coroutine
@resume:
incl Coroutine_reentrant(CO) ; coroutine.reentrant++
; setup bytecode env
movq rax, stack_frame_callee(rbp) ; rax = stack_frame.callee
movq rax, Closure_mai_fn(rax) ; rax = callee->mai_fn
movq rax, Function_bytecodes(rax) ; rax = callee->mai_fn->bytecodes
addq rax, BytecodeArray_instructions ; rax = &bytecodes->instructions
movq rbx, stack_frame_pc(rbp)
shll rbx, 2 ; pc * 4
addq rax, rbx
movq BC, rax
JUMP_NEXT_BC()

; Restore native stack and frame
@done:
; Unset root exception handler
movq rbx, rbp
subq rbx, stack_frame_caught_point
movq rax, CaughtNode_next(rbx) 
movq Coroutine_caught(CO), rax ; coroutine.caught = caught.next
; Recover system stack
movq rbp, Coroutine_bp(CO) ; recover system bp
movq rsp, Coroutine_sp(CO) ; recover system sp

; TODO:
```

* `InterpreterPump(Closure *callee)` 函数，调用一个字节码函数

```asm
; make a interpreter env
pushq rbp
movq rbp, rsp

movq rax, Closure_mai_fn(Argv_0) ; rax = callee->mai_fn
subq rsp, Function_stack_size(rax) ; rsp -= mai_fn->stack_size and keep rbp
movq stack_frame_maker(rbp), InterpreterFrame::kMaker
movq stack_frame_pc(rbp), 0 ; set pc = 0
movq stack_frame_callee(rbp), Coroutine_entry_point(rax) ; set callee
movq rbx, Function_const_pool(rax) ; rbx = mai_fn->const_pool
movq stack_frame_const_pool(rbp), rbx ; set const_pool
movq rbx, Function_tags(rax) ; test tags
test rbx, Function::kExceptionHandleBit ; if (mai_fn->has_execption_handle())
jz @start

; install caught handler
movq rbx, rbp
subq rbx, stack_frame_caught_point
movq Coroutine_caught(CO), rbx ; coroutine.caught = &caught
;movq CaughtNode_next(rbx), 0 ; caught.next = nullptr
movq CaughtNode_bp(rbx), rbp ; caught.bp = system rbp
movq CaughtNode_sp(rbx), rsp ; caught.sp = system rsp
movq rax, @uncaught_handler
movq CaughtNode_pc(rbx), rax ; caught.pc = @exception_dispatch
jmp near @start
nop
nop
... ; byte patches
nop

; exception caught dispatch 
@exception_dispatch:
movq SCRATCH, rax ; SCRATCH will be protectd by SwitchSystemStackCall
movq r11, Closure::DispatchException ; call->DispatchException(exception)
movq Argv_0, stack_frame_callee(rbp) ; argv[0] = callee
movq Argv_1, SCRATCH ; argv[1] = exception
call SwitchSystemStackCall ; switch system stack and call a c++ function
movq rbx, rax
cmpl rbx, 0
jl @throw_again ; if (retval < 0)?
jmp near 

@throw_again:
movq rbx, Coroutine_caught(CO)
movq rbx, Caught_pc(rbx)
movq rax, SCRATCH ; SCRATCH is saved to rax: current exception
jmp far rbx ; throw again to prev handler

; TODO:

; goto first bytecode handler
; the first bytecode can jump to second bytecode handler, and next and next next.
@start:
movq rax, stack_frame_callee(rbp)
movq rax, Closure_mai_fn(rax)
movq rax, Function_bytecodes(rax) ; rax = callee->mai_fn->bytecodes
addq rax, BytecodeArray_instructions ; rax = &bytecodes->instructions
movq BC, rax ; setup new BC
movq rax, 0(BC) ; get fist bytecode
andl rax, 0xff000000
shrl rax, 24
jmp far rbx*8(BC_ARRAY) ; jump to first bytecode handler

; keep rax, beasue it's return value.
movq rbx, stack_frame_callee(rbp)
movq rbx, Closure_mai_fn(rbx)
movq rcx, Function_tags(rbx) ; rcx = mai_fn->tags
test rcx, Function::kExceptionHandleBit ; if (mai_fn->has_execption_handle())
jz @done
; Uninstall caught handle
movq SCRATCH, Coroutine_caught(CO)

; TODO:

@done
addq rsp, Function_stack_size(rbx) ; recover stack
popq rbp
ret
```

* `SwitchSystemStackCall`函数，切换到系统栈执行一个C++函数。

```asm
; switch to system stack and call
pushq rbp
movq rbp, rsp

movq rax, rbp
movq rbx, rsp
movq rbp, Coroutine_bp(CO) ; recover system bp
movq rsp, Coroutine_sp(CO) ; recover system sp
pushq rax ; save mai sp
pushq rbx ; save mai bp
pushq SCRATCH
pushq CO
pushq BC
pushq BC_ARRAY

call r11 ; Call real function

; switch back to mai stack
popq BC_ARRAY
popq BC
popq CO
popq SCRATCH
popq rbx ; keep rax
popq rcx
movq rsp, rcx
movq rbp, rbx

popq rbp
ret
```



* `JUMP_NEXT_BC()` 宏，在解释器执行环境中，负责跳转到下一个`bytecode`的处理代码。

```asm
; move next BC and jump to handler
incl stack_frame_pc(rbp) ; bc++ go to next bc
addq BC, 4 ; BC++
movq rbx, 0(BC) ; rbx = BC[0]
andl rbx, 0xff000000
shrl rbx, 24 ; (bc & 0xff000000) >> 24
jmp far rbx*8(BC_ARRAY) ; jump to next bytecode handler
```

#### 字节码详细

* Load to ACC
    * 作用：将栈中数据放入寄存器A中
    * 类型：`A`型
    * 副作用：改变`RA`

```asm
; [ Ldar32/64 ]
movl rbx, 0(BC)
andl rbx, 0xffffff
| movl eax, rbx(rbp)
| movq rax, rbx(rbp)
JUMP_NEXT_BC()

; [ Ldaf32/64 ]
movl rbx, 0(BC)
andl rbx, 0xffffff
| movss xmm0, rbx(rbp)
| movsd xmm0, rbx(rbp)
JUMP_NEXT_BC()
```

* Store from ACC
    * 作用：将寄存器A中数据放入栈中
    * 类型：`A`型
    * 副作用：无

```asm
; [ Star32/64 ]
movl rbx, 0(BC)
andl rbx, 0xffffff
| movl rbx(rbp), eax
| movq rbx(rbp), rax
JUMP_NEXT_BC()

; [ Staf32/64 ]
movl rbx, 0(BC)
andl rbx, 0xffffff
| movss rbx(rbp), xmm0
| movsd rbx(rbp), xmm0
JUMP_NEXT_BC()
```

* Move Stack to Stack:
    * 作用：移动栈中数据
    * 类型：`AB`型
    * `参数A`：目的地址
    * `参数B`：源地址
    * 副作用：无

```asm
; [ Move32/64 ]
movl rbx, 0(BC)
andl rbx, 0xfff
| movl eax, rbx(rbp)
| movq rax, rbx(rbp)
movl rbx, NEXT_BC
andl rbx, 0xfff000
shrl rbx, 12
| movl rbx(rbp), eax
| movq rbx(rbp), rax
JUMP_NEXT_BC()
```

* Add Stack Values to ACC
    * 作用：栈中两个变量相加，结果放入`rax`中
    * 类型：`AB`型
    * `参数A`：加数
    * `参数B`：被加数
    * 副作用：写入`ACC`

```asm
; [ Add32/64 ]
movl rbx, 0(BC)
andl rbx, 0xfff000
shrl rbx, 12
| movl eax, rbx(rbp)
| movq rax, rbx(rbp)
movl rbx, NEXT_BC
andl rbx, 0xfff
| addl eax, rbx(rbp)
| addq rax, rbx(rbp)
JUMP_NEXT_BC()

; [ Addf32/64 ]
movl rbx, 0(BC)
andl rbx, 0xfff000
shrl rbx, 12
| movss xmm0, rbx(rbp)
| movsd xmm0, rbx(rbp)
movl rbx, NEXT_BC
andl rbx, 0xfff
| addss xmm0, rbx(rbp)
| addsd xmm0, rbx(rbp)
JUMP_NEXT_BC()
```

* Subtract Two Stack Values to ACC
    * 作用：栈中两个变量相减，结果放入`rax`中
    * 类型：`AB`型
    * `参数A`：被减数
    * `参数B`：减数
    * 副作用：写入`ACC`

```asm
; [ Sub32/64 ]
movl rbx, 0(BC)
andl rbx, 0xfff000
shrl rbx, 12
| movl eax, rbx(rbp)
| movq rax, rbx(rbp)
movl rbx, 0(BC)
andl rbx, 0xfff
| subl eax, rbx(rbp)
| subq rax, rbx(rbp)
JUMP_NEXT_BC()

; [ Subf32/64 ]
movl rbx, 0(BC)
andl rbx, 0xfff000
shrl rbx, 12
| movss xmm0, rbx(rbp)
| movsd xmm0, rbx(rbp)
movl rbx, 0(BC)
andl rbx, 0xfff
| subss xmm0, rbx(rbp)
| subsd xmm0, rbx(rbp)
JUMP_NEXT_BC()
```

* Multiply Two Stack Unsgined Values to ACC
    * 作用：栈中两个变量相减，结果放入`rax`中
    * 类型：`AB`型
    * `参数A`：被减数
    * `参数B`：减数
    * 副作用：写入`ACC`

```asm
; [ Mul32/64 ]
movl rbx, 0(BC)
andl rbx, 0xfff000
shrl rbx, 12
| movl eax, rbx(rbp)
| movq rax, rbx(rbp)
movl rbx, 0(BC)
andl rbx, 0xfff
| mull eax, rbx(rbp)
| mulq rax, rbx(rbp)
JUMP_NEXT_BC()

; [ Mulf32/64 ]
movl rbx, 0(BC)
andl rbx, 0xfff000
shrl rbx, 12
| movss xmm0, rbx(rbp)
| movsd xmm0, rbx(rbp)
movl rbx, 0(BC)
andl rbx, 0xfff
| mulss xmm0, rbx(rbp)
| mulsd xmm0, rbx(rbp)
JUMP_NEXT_BC()
```

* Call Function
    * 作用：调用一个函数，函数对象指针放在`rax`中
    * 类型：`AB`型
    * `参数A`：参数开始地址
    * `参数B`：参数结束地址
    * 副作用：写入`RA`或`XA`

```
; [ Call ]
...
```

* Call Native Function
    * 作用：调用一个`native`修饰函数，函数对象指针放在`rax`中
    * 类型：`AB`型
    * `参数A`：参数开始地址
    * `参数B`：参数结束地址
    * 副作用：写入`RA`或`XA`

```
; [ CallNative ]
...
```





