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



```scala
// while 循环
while (true) {
    // 死循环
}

while (val ok = check; ok) {
    // 当ok==true时循环
}

// for 循环
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

### 异常语句

和常见的java、kotlin异常处理语句相似

* `try` 子句
* `catch` 子句
* `finally` 子句

```BFN
throw_stmt := `throw' expr
try_stmt := `try' `{' block `}' { `catch' `{' block `}' } [ `finally' `{' block `}' ]
```



```scala
try {
    foo()
} catch (e: Error) {
    e.printBacktrace
} catch (e: Exception) {
    println("e: $e")
} catch (e: AppException) {
    println("app: $e")
} finally {
    println("final")
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

#### 堆空间



#### 栈空间

* * *

> 每个`routine`有一个自己的栈，参数全部通过栈传递，返回值通过寄存器传递(x64为`rax`)

```c++
struct Stack {
    Stack *next;
    Address guard0;
    Address guard1;
    Address hi;
    Address lo;
    size_t  size;
    uint8_t bytes[16]
};
```



* 调用者栈(`caller`)

```
字节码函数的调用栈

+-------------------+ --------- stack begin -------
| return address    | 8 bytes
+-------------------+
| saved bp          | 8 bytes -------->[ prev bp ]
+-------------------+ <================[ current bp ]
| maker             | 4 bytes
+-------------------+ --------
| pc                | 4 bytes
+-------------------+
| callee function   | 8 bytes
+-------------------+
| bytecode array    | 8 bytes
+-------------------+
| constants pool    | 8 bytes
+-------------------+
| caught node       | optional: 32 bytes
+-------------------+
| local var span[0] | local variable spans
+-------------------+
| local var span[1] | N * Span16
+-------------------+
| ... ... ... ...   |
+-------------------+ <================[ current sp ]
| argv[0]           | 4/8 bytes first argument
+-------------------+ 
| argv[1]           | 4/8 bytes secrond argument
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
| argv[1]           | 4/8 bytes first argument
+-------------------+ 
| argv[2]           | 4/8 bytes secrond argument
+-------------------+ 
| ... ... ... ...   | ...others arguments
+-------------------+ 
| vargs             | optional: 8 bytes
+-------------------+ --------- callee stack -------
| return address    | 8 bytes
+-------------------+
| saved bp          | 8 bytes -------->[ prev bp ]
+-------------------+ <================[ current bp ]
| maker             | 4 bytes
+-------------------+
| pc                | 4 bytes
+-------------------+
| callee function   | 8 bytes
+-------------------+
| bytecode array    | 8 bytes
+-------------------+
| constants pool    | 8 bytes
+-------------------+
| caught node       | optional: 32 bytes
+-------------------+ -------- local vars ----------
| local var span[0] | local variable spans
+-------------------+
| local var span[1] | N * Span16
+-------------------+
| ... ... ... ...   |
+-------------------+ <================[ current sp ]
```

### Handles

> `Handle`用于在C++环境引用堆对象，防止其被垃圾回收器意外回收。同时因为GC会移动堆对象，`Handle`需要间接引用引用对象，不能直接持有对象指针。

* `HandleScope`: 用于管理局部handle的声明周期；每个`HandleScope`只能在其所属的线程内有效。
* `Handle`：局部引用，生命周期由`HandleScope`管理。
* `Persistent`：全局引用，生命周期需要手动管理。

```
- HandleScope槽
- 每一个Machine对象会持有一个HandleScopeSlot链表，来管理本线程的Handle，因此不需要线程安全。
- 存储handle的内存按照page分配，然后每个slot拆分自己需要大小；如果需求超过1 page了，就再分配1 page。
+=============================+
[       HandleScopeSlot       ]
+=============================+
|         scope: HandleScope* | current HandleScope pointer
|-----------------------------|
|      prev: HandleScopeSlot* | previous slot pointer
|-----------------------------|
|               base: Address | handles begin address
|-----------------------------|
|                end: Address | handles end address 
|-----------------------------|
|              limit: Address | handles limit address(total end address)
|-----------------------------|

base == prev->end, 从上一个slot的end address开始为本slot分配内存。
                    page-1                           page-2
       [--------------------------------|-------------------------------]
slot-1 [==========]                     |
slot-2            [==========]          |
slot-3                       [=======]  |
                                        |
slot-4                                  [========]
slot-5                                  |        [==========]


- Persistent节点：使用双链表的形式存储persistent handle（全局句柄）
- Persistent节点链表保存在Isolate对象中，每个线程都可能访问到，因此必须是线程安全的。
+=============================+
[        PersistentNode       ]
+=============================+
|       next: PersistentNode* | next pointer for double-linkded list
|-----------------------------|
|       prev: PersistentNode* | next pointer for double-linkded list
|-----------------------------|
|             handle: Address | handle
|-----------------------------|
|                    tid: u32 | owner thread id
+-----------------------------+
```



```C++
class HandleScope {
public:
  	// Into this scope
    HandleScope(base::Initializer/*avoid c++ compiler optimize this object*/);
  	// Out this scope
    ~HandleScope();
    
    // New handle address and set heap object pointer
    void **NewHandle(void *value);
    
  	// Escape a owned local handle
    template<class T> inline Handle<T> CloseAndEscape(Handle<T> in_scope);
    
  	// Get the prev(scope) HandleScope 
    HandleScope *GetPrev();

  	// Get this scope's HandleScope
    static HandleScope *Current();
};

template<class T>
class HandleBase {
public:
    // Get heap object pointer, is value
    T *get() const;

    // Get handle address, not value
    T **location() const;

    // Test heap object pointer is null or not
    bool is_value_null() const;
    bool is_value_not_null() const;

    // Test handle address is null or not
    bool is_empty() const;
    bool is_not_empty() const;
protected:
    // Raw initialize handle
    inline HandleBase(T **location);
private:
    T **location_;
}; // class HandleBase

// Handle is thread local value, dont use it in other threads
template<class T>
class Handle : public HandleBase<T> {
public:
    // Initialize by a handle address
    inline explicit Handle(T **location = nullptr);

    // Initialize by heap object pointer
    template<class S> inline explicit Handle(S *pointer);

    // Initialize by other handle
    template<class S> inline explicit Handle(Handle<S> other);

    // Copy constructor
    inline Handle(const Handle &other);

    // Right reference constructor
    inline Handle(Handle &&other);

    // Getters
    T *operator -> () const;
    T *operator * () const;

    // Tester
    bool operator ! () const;
    bool operator == (const Handle &) const;

    // Assignment
    void operator = (const Handle &);
    void operator = (const Handle &&);
    void operator = (T *pointer);

    // Utils
    // Handle cast
    template<class S> static inline Handle<T> Cast(const Handle<S> other);
    // Make a null heap pointer handle(handle address is not null)
    static inline Handle<T> Null();
    // Make a null handle address handle
    static inline Handle<T> Empty();
}; // class Handle

template<class T>
class Persistent : public HandleBase<T> {
public:
    // Initialize a empty handle
    inline Persistent();
    // Copy constructor
    inline Persistent(const Persistent &);
    // Right reference constructor
    inline Persistent(Persistent &&);

    // Manual drop this handle, object mybe release by next GC
    inline void Dispose();
  
      // Getters
    T *operator -> () const;
    T *operator * () const;

    // Tester
    bool operator ! () const;
    bool operator == (const Handle &) const;

  	// Utils
    // Handle cast
    template<class S> static inline Persistent<T> Cast(const Persistent<S> other);
		// Create by local handle
  	static inline Persistent<T> New(const Handle<T> &local);
}; // class Persistent
```



```c++
String *Foo(String *arg0, mai::i32_t arg1, mai::f32_t arg2) {
    HandleScope handle_scope(base::ON_EXIT_SCOPE_INITIALIZER);
    
    printf("%d\n", arg1);
    printf("%f\n", arg2);
  
	Context *ctx = Context::Get();
    printf("%d\n", ctx->GetTid());
    return *String::NewUtf8("ok")
}

```

### Isolate和Context

* * *

> `Isolate`就是对虚拟机的抽象

```
- 对虚拟机的抽象
+==================================+
[              Isolate             ]
+==================================+
|                      heap: Heap* | Heap memory management
|----------------------------------|
|              metadata: Metadata* | Metadata space
|----------------------------------|
|       global_space: GlobalSpace* | global variables space
|----------------------------------|
|                context: Context* | context pointer
|----------------------------------|
| dummy_persistent: PersistentNode | persistent double-linked list header
|----------------------------------|
|           scheduler: Scheduler * | machines and coroutines management
|----------------------------------|
|               machine0: Machine* | The main os thread
+----------------------------------+

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
    if (auto err = context->Compile(/*be compiled code*/); err.fail()) {
        // handler errors
    }
  	HandleScope handle_scope(base::ON_EXIT_SCOPE_INITIALIZER);
  	std::vector<Handle<String>> vals;
  	for (int i = 0; i < argc; i++) {
        vals.push_back(String::NewUtf8(argv[i]));
    }
  	Handle<GenericArray> argv = GenericArray::New(vals);
  	context->SetArgv(argv);
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
|----------------------------|
| top_slot: HandleScopeSlot* | top handle scope slots, this is linked list for handle scope
|----------------------------|

- 对协程的抽象
-- 协程的状态:

Coroutine::kDead: 协程死亡，协程入口函数执行完后处于此状态
Coroutine::kIdle: 空闲，未被任何一个machine调度
Coroutine::kWaitting: 等待中，一般是等待读写channel或者文件读写
Coroutine::kRunnable: 可被执行
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
|------------------------------|
| waitting: WaittingQueueNode* | waitting node
|------------------------------|
|      exception: Exception* | native function thrown exception 
|----------------------------|
|        caught: CaughtNode* | exception hook for exception caught
|----------------------------|
|                 yield: u32 | yield requests, if yield > 0, need yield
|----------------------------|
|            sys_bp: Address | mai frame pointer
|----------------------------|
|            sys_sp: Address | mai stack pointer
|----------------------------|
|            sys_pc: Address | mai program counter
|----------------------------|
|                bp: Address | mai frame pointer
|----------------------------|
|                sp: Address | mai stack pointer
|----------------------------|
|                pc: Address | mai program counter
|----------------------------|
|                   acc: u64 | saved ACC
|----------------------------|
|                  xacc: f64 | saved XACC
|----------------------------|
|              stack: Stack* | function stack

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

### Channel

> `channel`本质是一个线程安全的FIFO的队列，有一个固定大小（默认大小为1），操作`channel`的时候回自动触发重新调度。
>
> 写入`channel`时，如果队列已满，将会放弃控制权，调度其他coroutine执行。
>
> 读取`channel`时，如果队列已空，也会放弃控制权，调度其他coroutine执行。
>
> 使用ring-buffer来实现`channel`

```
+================================+
[             Channel            ]
+================================+
|            Any Header          | heap object header
|--------------------------------|
|                    closed: u32 | is close ?
|--------------------------------|
|  send_queue: WaittingQueueNode | waitting-queue for send
|--------------------------------|
|  recv_queue: WaittingQueueNode | waitting-queue for recv
|--------------------------------|
|                    lock: mutex | lock
|--------------------------------|
|                  capacity: u32 | size of capacity
|--------------------------------|
|                       len: u32 | size of valid elements
|--------------------------------|
|                     start: i32 | start position
|--------------------------------|
|                       end: i32 | end position
|--------------------------------|


+==========================+
[     WaittingQueueNode    ]
+==========================+
| next: WaittingQueueNode* |
|--------------------------| double-linked list header
| prev: WaittingQueueNode* |
|--------------------------|
|           co: Coroutine* | pointer of coroutine
|--------------------------|
|        received: Address | direct received address



- 基础类型的Channel

+======================+
[  Channel8(Channdle)  ]
+======================+
|    Channel Header    | channel header
+----------------------+
|            buf: u8[] | buffer
+----------------------+

+======================+
[ Channel16(Channdle)  ]
+======================+
|    Channel Header    | channel header
+----------------------+
|           buf: u16[] | buffer
+----------------------+

+======================+
[ Channel32(Channdle)  ]
+======================+
|    Channel Header    | channel header
+----------------------+
|           buf: u16[] | buffer
+----------------------+

+======================+
[ Channel64(Channdle)  ]
+======================+
|    Channel Header    | channel header
+----------------------+
|           buf: u64[] | buffer
+----------------------+

+===========================+
[ GenericChannel(Channdle)  ]
+===========================+
|       Channel Header      | channel header
+---------------------------+
|               buf: Any*[] | buffer
+---------------------------+


Ring-Buffer:    |<------------- fixed size buffer ----------------->|
+---------------+---+---+---+---+---+---+---+---+---+---+---+---+---+
| channel heaer | v | v | v | v | v |   |   |   |   |   |   |   |   |
+---------------+---+---+---+---+---+---+---+---+---+---+---+---+---+
                  ^               ^
                start            end
+---------------+---+---+---+---+---+---+---+---+---+---+---+---+---+
| channel heaer | v | v | v |   |   |   |   |   |   |   |   | v | v |
+---------------+---+---+---+---+---+---+---+---+---+---+---+---+---+
                          ^                                   ^
                         end                                start

channle一般会跨线程共享，必须要实现线程安全。
入队列：
end = (end + 1) % capacity
buf[end] = value
在end处添加元素，然后后移end指针，当end和start重叠，表示队列已满。

出队列：
value = buf[start]
start = (start + 1) % capacity
在start处获取元素，然后后移start指针，当start和end重叠，表示队列已空。

队列中有效数据长度就是: abs(end - start)

end或者start如果超过capacity之后，要绕回
```

* 发送
    * 直接发送：接收等待队列`recv_queue`不为空；从中选出第一个陷入等待的`coroutine`来直接发送。
    * 缓冲发送：缓冲区没有满；此时将数据写入缓冲区。
    * 阻塞发送：发送的数据没法处理，接收队列为空、缓冲区已满；此时将当前`coroutine`放入发送等待队列，然后放弃控制权，触发重新调度。
* 接收
    * 直接接收：发送等待队列`send_queue`不为空；从中选出第一个陷入等待的`coroutine`来直接接收。
    * 缓冲接收：缓冲区中有数据；此时直接从缓冲区中移动数据到目的地址（移动到`coroutine->acc/xacc`）
    * 阻塞接收：发送等待队列`send_queue`为空，缓冲区中又没有数据；此时将当前`coroutine`放入接收等待队列，然后放弃控制权，触发重新调度。

### FunctionTemplate

> 函数模板用于包装C++的函数调用。生成一个汇编器生成stub，通过stub间接调用C++函数。

函数模板类：

```c++
class FunctionTemplate {
public:
    template<class T>
    struct ParameterTraits {
    }

    template<> struct ParameterTraits<int8_t> {
        static constexpr int kType = Type::I8
    }

    template<> struct ParameterTraits<uint8_t> {
        static constexpr int kType = Type::U8
    }

	template<R> static Handle<Closure> New(R(*ptr)(), int n);
  	template<R,A0> static Handle<Closure> New(R(*ptr)(A0), int n);
  	template<R,A0,A1> static Handle<Closure> New(R(*ptr)(A0,A1), int n);
		...
	template<R,A0,A1,A2,A3,A4> static inline Handle<Closure> New(R(*ptr)(A0,A1,A1,A2,A3,A4), int n) {
      	int params_types[5] = {
            ParameterTraits<A0>::kType,
            ParameterTraits<A1>::kType,
            ParameterTraits<A2>::kType,
            ParameterTraits<A3>::kType,
            ParameterTraits<A4>::kType,
        };
      	Code *code = Compiler::MakeNativeStub(Isolate::Get(), params_types, 5);
      	return Handle<Closure>::New(Closure::NewFromStub(code, n));
    }
};
```

* 使用示例：
* 注册C++函数：
```C++

String *BarHandle(i32_t n, f32_t m) {
    HandleScope handle_scope(INITIALIZER_LINKER);
    char buf[128];
    snprintf(buf, sizeof(buf), "%d+%f", n, m);
    return *String::NewUtf8(buf);
}

void foo() {
    HandleScope handle_scope(INITIALIZER_LINKER);

    Context *context = Context::Get();
    Handle<Closure> function = FunctionTemplate::New(BarHandle);
    context->RegisterGlobalFunction("main.bar", function);
}
```

* 生成的stub代码：
```asm
; [ stub for 'BarHandle' ]
push rbp
movq rbp, rsp
subq rsp, 16 ; StubStackFrame::kSize
movl stack_frame_maker(rbp), StubStackFrame::kMaker

; +4 argv_1
; +8 padding
; +8 return address
; +8 saved bp
; stub stack frame
movl Argv_0, 28(rbp) ; argv[0] 16 + 12
movss xmm0, 24(rbp) ; argv[1] 16 + 8
movq r11, BarHandle
call SwitchSystemStackCall

addq rsp, 16 ; StubStackFrame::kSize
popq rbp
ret
```

* 在mai代码中调用C++函数：

```scala
// file: main/main.mai
package main

// implement by c++
native def bar(n:i32, m:f32):string

def main() {
    // invoke c++ native function
    println(bar(1, .1))
}
```



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
|                Any Header               | heap object header
+-----------------------------------------+
|   cxx_fn: Code*   |  mai_fn :Function*  | function proto or cxx function stub
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

- FA型：有2个参数

|< 8b >|<-- 8 bits -->|<--------- 16 bits --------->|
+------+--------------+--------------+--------------+
| code |     Flags    |         Parameter A         |
+------+--------------+--------------+--------------+
```

#### 命令代码


| Code                | 编码类型 | 作用 | 参数F/A                     | 参数B          |
| ------------------- | -------: | ---- | --------------------------- | -------------- |
| `Ldar32` | `A` | 读取数据到ACC | `u24` 栈偏移量 | |
| `Ldar64` | `A` | 读取数据到ACC | `u24` 栈偏移量 | |
| `LdarPtr` | `A` | 读取数据到ACC | `u24` 栈偏移量 | |
| `Ldaf32` | `A` | 读取数据到ACC | `u24` 栈偏移量 | |
| `Ldaf64` | `A` | 读取数据到ACC | `u24` 栈偏移量 | |
| `LdaZero` | `N` | 读取0到ACC | | |
| `LdaSmi32` | `A` | 读取立即数到ACC | | |
| `LdaTrue` | `N` | 读取`true`到ACC | | |
| `LdaFalse` | `N` | 读取`false`到ACC | | |
| `Ldak32` | `A` | 读取常数到ACC | `u24` 常量池偏移量 | |
| `Ldak64` | `A` | 读取常数到ACC | `u24` 常量池偏移量 | |
| `Ldakf32` | `A` | 读取常数到ACC | `u24` 常量池偏移量 | |
| `Ldakf64` | `A` | 读取常数到ACC | `u24` 常量池偏移量 | |
| `LdaGlobal32` | `A` | 读取全局变量到ACC | `u24` `global_space` 偏移量 | |
| `LdaGlobal64` | `A` | 读取全局变量到ACC | `u24` `global_space` 偏移量 | |
| `LdaGlobalPtr` | `A` | 读取全局变量到ACC | `u24` `global_space` 偏移量 | |
| `LdaGlobalf32` | `A` | 读取全局变量到ACC | `u24` `global_space` 偏移量 | |
| `LdaGlobalf64` | `A` | 读取全局变量到ACC | `u24` `global_space` 偏移量 | |
| `LdaProperty8` | `A` | 读取对象属性到ACC | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `LdaProperty16` | `A` | 读取对象属性到ACC | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `LdaProperty32` | `A` | 读取对象属性到ACC | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `LdaProperty64` | `A` | 读取对象属性到ACC | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `LdaPropertyPtr` | `A` | 读取对象属性到ACC | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `LdaPropertyf32` | `A` | 读取对象属性到ACC | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `LdaPropertyf64` | `A` | 读取对象属性到ACC | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `LdaArrayElem8` | `AB` | 读取数组元素到ACC | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `LdaArrayElem16` | `AB` | 读取数组元素到ACC | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `LdaArrayElem32` | `AB` | 读取数组元素到ACC | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `LdaArrayElem64` | `AB` | 读取数组元素到ACC | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `LdaArrayElemPtr` | `AB` | 读取数组元素到ACC | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `LdaArrayElemf32` | `AB` | 读取数组元素到ACC | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `LdaArrayElemf64` | `AB` | 读取数组元素到ACC | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Star32` | `A` | 写入数据到栈 | `u24` 栈偏移量 | |
| `Star64` | `A` | 写入数据到栈 | `u24` 栈偏移量 | |
| `StarPtr` | `A` | 写入数据到栈 | `u24` 栈偏移量 | |
| `Staf32` | `A` | 写入数据到栈 | `u24` 栈偏移量 | |
| `Staf64` | `A` | 写入数据到栈 | `u24` 栈偏移量 | |
| `StaProperty8` | `A` | 写入对象属性 | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `StaProperty16` | `A` | 写入对象属性 | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `StaProperty32` | `A` | 写入对象属性 | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `StaProperty64` | `A` | 写入对象属性 | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `StaPropertyPtr` | `A` | 写入对象属性 | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `StaPropertyf32` | `A` | 写入对象属性 | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `StaPropertyf64` | `A` | 写入对象属性 | `u12` 栈偏移量 | `u12` 立即偏移量 |
| `StaArrayElem8` | `AB` | 写入数组元素 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `StaArrayElem16` | `AB` | 写入数组元素 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `StaArrayElem32` | `AB` | 写入数组元素 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `StaArrayElem64` | `AB` | 写入数组元素 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `StaArrayElemPtr` | `AB` | 写入数组元素 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `StaArrayElemf32` | `AB` | 写入数组元素 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `StaArrayElemf64` | `AB` | 写入数组元素 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Move32` | `AB` | 栈中移动数据 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Move64` | `AB` | 栈中移动数据 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `MovePtr` | `AB` | 栈中移动数据 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Truncate32To8` | `A` | 截断整数 | `u24` 栈偏移量 | |
| `Truncate32To16` | `A` | 截断整数 | `u24` 栈偏移量 | |
| `Truncate64To32` | `A` | 截断整数 | `u24` 栈偏移量 | |
| `ZeroExtend8To32` | `A` | 0扩展整数 | `u24` 栈偏移量 | |
| `ZeroExtend16To32` | `A` | 0扩展整数 | `u24` 栈偏移量 | |
| `ZeroExtend32To64` | `A` | 0扩展整数 | `u24` 栈偏移量 | |
| `SignExtend8To32` | `A` | 符号扩展整数 | `u24` 栈偏移量 | |
| `SignExtend16To32` | `A` | 符号扩展整数 | `u24` 栈偏移量 | |
| `SignExtend32To64` | `A` | 符号扩展整数 | `u24` 栈偏移量 | |
| `F32ToI32` | `A` | 浮点转整数 | `u24` 栈偏移量 | |
| `F64ToI32` | `A` | 浮点转整数 | `u24` 栈偏移量 | |
| `F32ToU32` | `A` | 浮点转整数 | `u24` 栈偏移量 | |
| `F64ToU32` | `A` | 浮点转整数 | `u24` 栈偏移量 | |
| `F32ToI64` | `A` | 浮点转整数 | `u24` 栈偏移量 | |
| `F64ToI64` | `A` | 浮点转整数 | `u24` 栈偏移量 | |
| `F32ToU64` | `A` | 浮点转整数 | `u24` 栈偏移量 | |
| `F64ToU64` | `A` | 浮点转整数 | `u24` 栈偏移量 | |
| `I32ToF32` | `A` | 整数转浮点 | `u24` 栈偏移量 | |
| `U32ToF32` | `A` | 整数转浮点 | `u24` 栈偏移量 | |
| `I64ToF32` | `A` | 整数转浮点 | `u24` 栈偏移量 | |
| `U64ToF32` | `A` | 整数转浮点 | `u24` 栈偏移量 | |
| `I32ToF64` | `A` | 整数转浮点 | `u24` 栈偏移量 | |
| `U32ToF64` | `A` | 整数转浮点 | `u24` 栈偏移量 | |
| `I64ToF64` | `A` | 整数转浮点 | `u24` 栈偏移量 | |
| `U64ToF64` | `A` | 整数转浮点 | `u24` 栈偏移量 | |
| `F64ToF32` | `A` | 浮点转浮点 | `u24` 栈偏移量 | |
| `F32ToF64` | `A` | 浮点转浮点 | `u24` 栈偏移量 | |
| `Add32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Add64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Addf32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Addf64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Sub32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Sub64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Subf32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Subf64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Mul32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Mul64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Mulf32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Mulf64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `IMul32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `IMul64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Div32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Div64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Divf32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Divf64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `IDiv32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `IDiv64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Mod32` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `Mod64` | `AB` | | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `BitwiseOr32` | `AB` | 按位或 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `BitwiseOr64` | `AB` | 按位或 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `BitwiseAnd32` | `AB` | 按位与 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `BitwiseAnd64` | `AB` | 按位与 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `BitwiseNot32` | `AB` | 按位非 | `u24` 栈偏移量 | |
| `BitwiseNot64` | `AB` | 按位非 | `u24` 栈偏移量 | |
| `BitwiseShl32` | `AB` | 按位左移 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `BitwiseShl64` | `AB` | 按位左移 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `BitwiseShr32` | `AB` | 按位右移 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `BitwiseShr64` | `AB` | 按位右移 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `BitwiseLogicShr32` | `AB` | 按位逻辑右移 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `BitwiseLogicShr64` | `AB` | 按位逻辑右移 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestEqual32` | `AB` | 测试相等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestNotEqual32` | `AB` | 测试不等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestLessThan32` | `AB` | 测试小于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestLessThanOrEqual32` | `AB` | 测试小于等于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestGreaterThan32` | `AB` | 测试大于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestGreaterThanEqual32` | `AB` | 测试大于等于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestEqual64` | `AB` | 测试相等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestNotEqual64` | `AB` | 测试不等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestLessThan64` | `AB` | 测试小于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestLessThanOrEqual64` | `AB` | 测试小于等于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestGreaterThan64` | `AB` | 测试大于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestGreaterThanEqual64` | `AB` | 测试大于等于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestEqualf32` | `AB` | 测试相等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestNotEqualf32` | `AB` | 测试不等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestLessThanf32` | `AB` | 测试小于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestLessThanOrEqualf32` | `AB` | 测试小于等于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestGreaterThanf32` | `AB` | 测试大于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestGreaterThanEqualf32` | `AB` | 测试大于等于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestEqualf64` | `AB` | 测试相等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestNotEqualf64` | `AB` | 测试不等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestLessThanf64` | `AB` | 测试小于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestLessThanOrEqualf64` | `AB` | 测试小于等于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestGreaterThanf64` | `AB` | 测试大于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestGreaterThanEqualf64` | `AB` | 测试大于等于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestStringEqual` | `AB` | 测试相等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestStringNotEqual` | `AB` | 测试不等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestStringLessThan` | `AB` | 测试小于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestStringLessThanOrEqual` | `AB` | 测试小于等于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestStringGreaterThan` | `AB` | 测试大于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestStringGreaterThanEqual` | `AB` | 测试大于等于 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestPtrEqual` | `AB` | 测试相等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestPtrNotEqual` | `AB` | 测试不等 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestIn` | `AB` | 测试元素是否在容器内 | `u12` 栈偏移量 | `u12` 栈偏移量 |
| `TestIs` | `AB` | 测试类型 | `u12` 栈偏移量 | `u12` 类型常量偏移 |
| `Throw` | `N` | 抛出异常 | | |
| `Yield` | `A` | 让出控制权 | `u24` 控制代码 | |
| `Goto` | `A` | 跳转到绝对位置 | `u24` PC位置 | |
| `JumpForward` | `A` | 向前跳转相对位置 | `u24`PC偏移量 | |
| `JumpBackward` | `A` | 向后跳转相对位置 | `u24`PC偏移量 | |
| `GotoIfTrue` | `A` | 测试结果为`true`跳转到绝对位置 | `u24`PC偏移量 | |
| `GotoIfFalse` | `A` | 测试结果为`false`跳转到绝对位置 | `u24`PC偏移量 | |
| `CallBytecodeFunction` | `A` | 调用函数 | `u24`栈偏移量 | |
| `CallNativeFunction` | `A` | 调用函数 | `u24`栈偏移量 | |
| `CallVtableFunction` | `A` | 调用函数 | `u12`栈偏移量 | |
| `Return` | `N` | 调用返回 | | |
| `NewBuiltinObject` | `FA` | 创建内建对象 | `u8`对象代码 | `u16`立即数 |
| `CheckStack` | `N` | 检查栈是否溢出 | | |

类型转换：任意两类型间，最多只需要两步转换：
规则：

1. 非`i32`,`u32`类型，先按有无符号，转换到`i32`，`u32`。
2. 再从`i32`,`u32`类型转换到目标类型。

* From `i8`, To:
    * `i8` -> None
    * `u8` -> None
    * `i16` -> `SignExtend8To32` + `Truncate32To8`
    * `u16` -> `ZeroExtend8To32` + `Truncate32To8`
    * `i32` -> `SignExtend8To32`
    * `u32` -> `ZeroExtend8To32`
    * `i64` -> `SignExtend8To32` + `SignExtend32To64`
    * `u64` -> `ZeroExtend8To32` + `ZeroExtend32To64`
    * `f32` -> `SignExtend8To32` + `I32ToF32`
    * `f64` -> `SignExtend8To32` + `I32ToF64`
* From `u8`, To:
    * `i8` -> None
    * `u8` -> None
    * `i16` -> `SignExtend8To32` + `Truncate32To8`
    * `u16` -> `ZeroExtend8To32` + `Truncate32To8`
    * `i32` -> `SignExtend8To32`
    * `u32` -> `ZeroExtend8To32`
    * `i64` -> `SignExtend8To32` + `SignExtend32To64`
    * `u64` -> `ZeroExtend8To32` + `ZeroExtend32To64`
    * `f32` -> `SignExtend8To32` + `I32ToF32`
    * `f64` -> `SignExtend8To32` + `I32ToF64`
* From i32, To:
    * `i8` -> `Truncate32To8`
    * `u8` -> `Truncate32To8`
    * `i16` -> `Truncate32To16`
    * `u16` -> `Truncate32To16`
    * `i32` -> None
    * `u32` -> None
    * `i64` -> `SignExtend32To64`
    * `u64` -> `ZeroExtend32To64`
    * `f32` -> `I32ToF32`
    * `f64` -> `I32ToF64`
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

* `Trampoline(Coroutine *co)`: 字节码执行的入口，负责从外部环境进入字节码执行环境。

```asm
; Prototype: void Trampoline(Coroutine *co)
; Save registers
; TODO:
pushq rbp
movq rbp, rsp

; Set bytecode handlers array
movq BC_ARRAY, ... ; TODO

; Save system stack and frame
movq CO, Argv_0
movq Coroutine_sys_bp(CO), rbp
movq Coroutine_sys_sp(CO), rsp
movq Coroutine_sys_pc(CO), @suspend

; if (coroutine.reentrant > 0) setup root exception is not need
cmpl Coroutine_reentrant(CO), 0
jg @entry

; Set root exception handler
subq SCRATCH, stack_frame_caught_point(rbp)
movq Coroutine_caught(CO), SCRATCH ; coroutine.caught = &caught
movq CaughtNode_next(SCRATCH), 0 ; caught.next = nullptr
movq CaughtNode_bp(SCRATCH), rbp ; caught.bp = system rbp
movq CaughtNode_sp(SCRATCH), rsp ; caught.sp = system rsp
movq rax, @uncaught_handler
movq CaughtNode_pc(SCRATCH), rax ; caught.pc = @exception_handler
jmp near @entry
nop
nop
... ; byte patches
nop

; Handler root exception
uncaught_handler: ; rax = top exception object
; switch to system stack
movq r11, Coroutine::Uncaught
movq Argv_0, CO
movq Argv_1, rax
call SwitchSystemStackCall; call co->Uncaught(exception)
jmp near @done

; function entry
; SCRATCH = coroutine
entry:
movq rbp, Coroutine_bp(CO) ; recover mai stack
movq rsp, Coroutine_sp(CO) ; recover mai stack
movl Coroutine_yield(CO) 0 ; coroutine.yield = 0
cmpl Coroutine_reentrant(CO), 0
jg @resume ; if (coroutine->reentrant > 0) 
; first calling
incl Coroutine_reentrant(CO) ; coroutine.reentrant++
movq Argv_0, Coroutine_entry_point(CO)
call InterpreterPump
jmp far @uninstall

suspend:
movq r11, Coroutine::Suspend
movq Argv_0, CO
movq Argv_1, rax
call SwitchSystemStackCall; call co->Suspend(acc, xacc)
jmp near @done

; coroutine->reentrant > 0, means: should resume this coroutine
resume:
incl Coroutine_reentrant(CO) ; coroutine.reentrant++
; setup bytecode env
movq SCRATCH, stack_frame_bytecode_array(rbp) ; SCRATCH = bytecode array
addq SCRATCH, BytecodeArray_instructions ; SCRATCH = &bytecodes->instructions
movq rbx, stack_frame_pc(rbp)
leaq BC, rbx*4(SCRATCH) ; [SCRATCH + rbx * 4]
movq rax, Coroutine_acc(CO) ; recover mai ACC
movsd xmm0, Coroutine_xacc(CO) ; recover mai XACC
JUMP_NEXT_BC()

; Restore native stack and frame
; Unset root exception handler
uninstall:
leaq rbx, stack_frame_caught_point(rbp)
movq rax, CaughtNode_next(rbx) 
movq Coroutine_caught(CO), rax ; coroutine.caught = caught.next

; Recover system stack
done:
movq rbp, Coroutine_sys_bp(CO) ; recover system bp
movq rsp, Coroutine_sys_sp(CO) ; recover system sp

; TODO:
popq rbp
ret
```

* `InterpreterPump(Closure *callee)` 函数，调用一个字节码函数

```asm
; Prototype: InterpreterPump(Closure *callee)
; make a interpreter env
pushq rbp
movq rbp, rsp

movq SCRATCH, Closure_mai_fn(Argv_0) ; SCRATCH = callee->mai_fn
subq rsp, Function_stack_size(SCRATCH) ; rsp -= mai_fn->stack_size and keep rbp
movq stack_frame_maker(rbp), InterpreterStackFrame::kMaker
movq stack_frame_pc(rbp), 0 ; set pc = 0
movq stack_frame_callee(rbp), Argv_0 ; set callee
movq rbx, Function_bytecodes(SCRATCH) ; rbx = mai_fn->bytecodes
movq stack_frame_bytecodearray(rbp), rbx ; set bytecode array
movq rbx, Function_const_pool(SCRATCH) ; rbx = mai_fn->const_pool
movq stack_frame_const_pool(rbp), rbx ; set const_pool
test Function_tags(SCRATCH), Function::kExceptionHandleBit ; if (mai_fn->has_execption_handle())
jz @start

; install caught handler
leaq SCRATCH, stack_frame_caught_point(rbp)
movq rax, Coroutine_caught(CO) ; rax = next caught
movq Coroutine_caught(CO), SCRATCH ; coroutine.caught = &caught
movq CaughtNode_next(SCRATCH), rax ; caught.next = coroutine.caught
movq CaughtNode_bp(SCRATCH), rbp ; caught.bp = system rbp
movq CaughtNode_sp(SCRATCH), rsp ; caught.sp = system rsp
movq rax, @exception_dispatch
movq CaughtNode_pc(SCRATCH), rax ; caught.pc = @exception_dispatch
jmp near @start
nop
nop
... ; byte patches
nop

; exception caught dispatch 
exception_dispatch:
movq SCRATCH, rax ; SCRATCH will be protectd by SwitchSystemStackCall
movq r11, Closure::DispatchException ; callee->DispatchException(exception, pc)
movq Argv_0, stack_frame_callee(rbp) ; argv[0] = callee
movq Argv_1, SCRATCH ; argv[1] = exception
movl Argv_2, stack_frame_pc(rbp)
call SwitchSystemStackCall ; switch system stack and call a c++ function
cmpl rax, 0
jl @throw_again ; if (retval < 0)?
; Do dispatch: rax is destination pc
movq stack_frame_pc(rbp), rax ; update pc
leaq SCRATCH, stack_frame_caught_point(rbp)
movq rbp, CaughtNode_bp(SCRATCH)
movq rsp, CaughtNode_sp(SCRATCH)
START_BC()

; uncaught exception, should throw again
throw_again:
movq rbx, Coroutine_caught(CO)
movq rax, CaughtNode_next(rbx)
movq Coroutine_caught(CO), rax ; coroutine.caught = coroutine.caught.next
movq rbx, CaughtNode_pc(rbx)
movq rax, SCRATCH ; SCRATCH is saved to rax: current exception
jmp far rbx ; throw again to prev handler

; goto first bytecode handler
; the first bytecode can jump to second bytecode handler, and next and next next.
start:
START_BC()
; never goto this
int 3
```

* `SwitchSystemStackCall`函数，切换到系统栈执行一个C++函数。

```asm
; switch to system stack and call
pushq rbp
movq rbp, rsp
subq rsp, 16 ; StubStackFrame::kSize
movl stack_frame_maker(rbp), StubStackFrame::kMaker

movq rax, rbp
movq rbx, rsp
movq rbp, Coroutine_sys_bp(CO) ; recover system bp
movq rsp, Coroutine_sys_sp(CO) ; recover system sp
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

addq rsp, 16 ; StubStackFrame::kSize
popq rbp
ret
```

* `START_BC()` 宏，在解释器执行环境中，开始执行当前PC的`bytecode` 处理代码

```asm
; jump to current pc's handler
movq SCRATCH, stack_frame_bytecode_array(rbp) ; SCRATCH = callee->mai_fn->bytecodes
addq SCRATCH, BytecodeArray_instructions ; SCRATCH = &bytecodes->instructions
movq rbx, stack_frame_pc(rbp)
leaq BC, rbx*4(SCRATCH) ; [SCRATCH + rbx * 4]
movl ebx, 0(BC) ; get fist bytecode
andl ebx, 0xff000000
shrl ebx, 24
jmp far rbx*8(BC_ARRAY) ; [BC_ARRAY + rbx * 8] jump to first bytecode handler
```

* `JUMP_NEXT_BC()` 宏，在解释器执行环境中，负责跳转到下一个`bytecode`的处理代码。

```asm
; move next BC and jump to handler
incl stack_frame_pc(rbp) ; pc++ go to next bc
addq BC, 4 ; BC++
movq rbx, 0(BC) ; rbx = BC[0]
andl rbx, 0xff000000
shrl rbx, 24 ; (bc & 0xff000000) >> 24
jmp far rbx*8(BC_ARRAY) ; jump to next bytecode handler
```

* `PANIC(err_code, msg)`宏，在解释器环境中，快速抛出致命异常，结束协程执行。

```asm
; PANIC(err_code, msg)
movq r11, Runtime::NewError
movl Argv_0, $err_code
movq Argv_1, $msg
call SwitchSystemStackCall ; Runtime::NewError(err_code, msg)
movq SCRATCH, Coroutine_caught(CO)
movq rbx, CaughtNode_pc(SCRATCH)
jmp far rbx
```

#### 字节码详细

* Load to ACC
    * 作用：将栈中数据放入ACC中
    * 类型：`A`型
        * `参数A`：栈偏移量
    * 副作用：写入`ACC`

```asm
; [ Ldar32/64/Ptr ]
movl ebx, 0(BC)
andl ebx, 0xffffff
negq rbx
| movl eax, rbx(rbp) ; Ldar32
| movq rax, rbx(rbp) ; Ldar64/LdarPtr
JUMP_NEXT_BC()

; [ Ldaf32/64 ]
movl ebx, 0(BC)
andl ebx, 0xffffff
negq rbx
| movss xmm0, rbx(rbp)
| movsd xmm0, rbx(rbp)
JUMP_NEXT_BC()
```

* Load property to ACC
    * 作用：读取栈中对象的属性放入ACC中
    * 类型：`AB`型
        * `参数A`：对象地址的`u12` 栈偏移量
        * `参数B`：对象属性的`u12`立即偏移量
    * 副作用：写入ACC

```asm
; [ LdaProperty8/16/32/64/Ptr/f32/f64 ]
; SCRATCH
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl ebx, 12
negq rbx
movq SCRATCH, rbx(rbp)
movl ebx, 0(BC)
andl ebx, 0xfff
| movb al, rbx(SCRATCH)    ; LdaProperty8
| movw ax, rbx(SCRATCH)    ; LdaProperty16
| movl eax, rbx(SCRATCH)   ; LdaProperty32
| movq rax, rbx(SCRATCH)   ; LdaProperty64/LdaPropertyPtr
| movss xmm0, rbx(SCRATCH) ; LdaPropertyf32
| movsd xmm0, rbx(SCRATCH) ; LdaPropertyf64
JUMP_NEXT_BC()
```

* Load array element to ACC
    * 作用：读取数组的元素放入ACC中
    * 类型：`AB`型
        * `参数A`：数组地址的`u12` 栈偏移量
        * `参数B`：下标地址的`u12` 栈偏移量
    * 副作用：写入ACC

```asm
; [ LdaArrayElem8/16/32/64/Ptr/f32/f64 ]
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl ebx, 12
negq rbx
movq SCRATCH, rbx(rbp) ; SCRATCH = array
movl ebx, 0(BC)
andl ebx, 0xfff
negq rbx
movl eax, rbx(rbp) ; index
cmpl eax, Array_len(SCRATCH) ; if (index >= array.len)
jge @out_of_bound
xorq rax, rax ; clear ACC
| movb al, rbx*1+Array_elems(SCRATCH) ; [ SCRATCH + rbx * 1 + Array_elems ] LdaArrayElem8
| movw ax, rbx*2+Array_elems(SCRATCH) ; [ SCRATCH + rbx * 2 + Array_elems ] LdaArrayElem16
| movl eax, rbx*4+Array_elems(SCRATCH) ; [ SCRATCH + rbx * 4 + Array_elems ] LdaArrayElem32
| movq rax, rbx*8+Array_elems(SCRATCH) ; [ SCRATCH + rbx * 8 + Array_elems ] LdaArrayElem64/Ptr
| movss xmm0, rbx*4+Array_elems(SCRATCH) ; [ SCRATCH + rbx * 4 + Array_elems ] LdaArrayElemf32
| movsd xmm0, rbx*8+Array_elems(SCRATCH) ; [ SCRATCH + rbx * 8 + Array_elems ] LdaArrayElemf64
JUMP_NEXT_BC()

out_of_bound:
PANIC(ERROR_OUT_OF_BOUND, "array out of bound!")
; Never goto this
int 3
```

* Store property by ACC
    * 作用：读取栈中对象的属性放入ACC中
    * 类型：`AB`型
        * `参数A`：对象地址的`u12` 栈偏移量
        * `参数B`：对象属性的`u12`立即偏移量
    * 副作用：写入ACC

```asm
; [ StaProperty8/16/32/64/Ptr/f32/f64 ]
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl eax, 12
neg eax
movq SCRATCH, rbx(rbp)
movl ebx, 0(BC)
andl ebx, 0xfff
| movb rbx(SCRATCH), al    ; StaProperty8
| movw rbx(SCRATCH), ax    ; StaProperty16
| movl rbx(SCRATCH), eax   ; StaProperty32
| movq rbx(SCRATCH), rax   ; StaProperty64
| | ; TODO: StaPropertyPtr write-barrier
| movss rbx(SCRATCH), xmm0 ; StaPropertyf32
| movsd rbx(SCRATCH), xmm0 ; StaPropertyf64
JUMP_NEXT_BC()
```

* Store array element by ACC
    * 作用：将ACC中数据写入数组元素
    * 类型：`AB`型
        * `参数A`：数组地址的`u12` 栈偏移量
        * `参数B`：下标地址的`u12` 栈偏移量
    * 副作用：无

```asm
; [ StaArrayElem8/16/32/64/Ptr/f32/f64 ]
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl ebx, 12
negq rbx
movq SCRATCH, rbx(rbp) ; SCRATCH = array
movl ebx, 0(BC)
andl ebx, 0xfff
negq rbx
movl eax, rbx(rbp) ; index
cmpl eax, Array_len(SCRATCH) ; if (index >= array.len)
jge @out_of_bound
addq SCRATCH, Array_elems ; SCRATCH = &elems[0]
| movb rbx*1(SCRATCH), al ; [ SCRATCH + rbx * 1 ] StaArrayElem8
| movw rbx*2(SCRATCH), ax ; [ SCRATCH + rbx * 2 ] StaArrayElem16
| movl rbx*4(SCRATCH), eax ; [ SCRATCH + rbx * 4 ] StaArrayElem32
| movq rbx*8(SCRATCH), rax ; [ SCRATCH + rbx * 8 ] StaArrayElem64
| | ; TODO: StaArrayElemPtr write-barrier
| movss rbx*4(SCRATCH), xmm0 ; [ SCRATCH + rbx * 4 ] StaArrayElemf32
| movsd rbx*8(SCRATCH), xmm0 ; [ SCRATCH + rbx * 8 ] StaArrayElemf64
JUMP_NEXT_BC()

out_of_bound:
PANIC(ERROR_OUT_OF_BOUND, "array out of bound!")
; Never goto this
int 3
```

* Store from ACC
    * 作用：将寄存器A中数据放入栈中
    * 类型：`A`型
        * `参数A`：源地址的栈偏移量
    * 副作用：无

```asm
; [ Star32/64/Ptr/f32/f64 ]
movl ebx, 0(BC)
andl ebx, 0xffffff
| movl rbx(rbp), eax ; Star32
| movq rbx(rbp), rax ; Star64/StarPtr
| movss rbx(rbp), xmm0 ; Staf32
| movsd rbx(rbp), xmm0 ; Staf64
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
movl ebx, 0(BC)
andl ebx, 0xfff
| movl eax, rbx(rbp) ; Move32
| movq rax, rbx(rbp) ; Move64/MovePtr
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl ebx, 12
| movl rbx(rbp), eax ; Move32
| movq rbx(rbp), rax ; Move64/MovePtr
JUMP_NEXT_BC()
```

* Add Stack Values to ACC
    * 作用：栈中两个变量相加，结果放入`rax`中
    * 类型：`AB`型
        * `参数A`：被加数在栈中的偏移量
        * `参数B`：加数在栈中的偏移量
    * 副作用：写入`ACC`

```asm
; [ Add32/64/f32/f64 ]
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl ebx, 12
| movl eax, rbx(rbp) ; Add32
| movq rax, rbx(rbp) ; Add64
| movss xmm0, rbx(rbp) ; Addf32
| movsd xmm0, rbx(rbp) ; Addf64
movl ebx, 0(BC)
andl ebx, 0xfff
| addl eax, rbx(rbp) ; Add32
| addq rax, rbx(rbp) ; Add64
| addss xmm0, rbx(rbp) ; Addf32
| addsd xmm0, rbx(rbp) ; Addf64
JUMP_NEXT_BC()
```

* Subtract Two Stack Values to ACC
    * 作用：栈中两个变量相减，结果放入`rax`中
    * 类型：`AB`型
    * `参数A`：被减数在栈中的偏移量
    * `参数B`：减数在栈中的偏移量
    * 副作用：写入`ACC`

```asm
; [ Sub32/64/f32/f64 ]
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl ebx, 12
| movl eax, rbx(rbp) ; Sub32
| movq rax, rbx(rbp) ; Sub64
| movss xmm0, rbx(rbp) ; Subf32
| movsd xmm0, rbx(rbp) ; Subf64
movl ebx, 0(BC)
andl ebx, 0xfff
| subl eax, rbx(rbp) ; Sub32
| subq rax, rbx(rbp) ; Sub64
| subss xmm0, rbx(rbp) ; Subf32
| subsd xmm0, rbx(rbp) ; Subf64
JUMP_NEXT_BC()
```

* Multiply Two Stack Values to ACC
    * 作用：栈中两个数相乘，结果放入ACC中
    * 类型：`AB`型
        * `参数A`：被乘数在栈中的偏移量
        * `参数B`：倍数在栈中的偏移量
    * 副作用：写入`ACC`

```asm
; [ Mul32/64/f32/f64 IMul32/64 ]
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl ebx, 12
| movl eax, rbx(rbp) ; Mul32/IMul32
| movq rax, rbx(rbp) ; Mul64/IMul64
| movss xmm0, rbx(rbp) ; Mulf32
| movsd xmm0, rbx(rbp) ; Mulf64
movl ebx, 0(BC)
andl ebx, 0xfff
| mull eax, rbx(rbp) ; Mul32
| mulq rax, rbx(rbp) ; Mul64
| imull eax, rbx(rbp) ; IMul32
| imulq rax, rbx(rbp) ; IMul64
| mulss xmm0, rbx(rbp) ; Mulf32
| mulsd xmm0, rbx(rbp) ; Mulf64
JUMP_NEXT_BC()
```

* Divide Two Stack Values to ACC
    * 作用：栈中两个数相除，结果放入ACC中
    * 类型：`AB`型
        * `参数A`：被除数在栈中的偏移量
        * `参数B`：倍数在栈中的偏移量，如果为0，会抛出错误
    * 副作用：写入`ACC`

```asm
; [ Div32/64/f32/f64 IDiv32/64 ]
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl ebx, 12
| cmpl rbx(rbp), 0 ; Mul32/IMul32
| | je @error
| | movl eax, rbx(rbp)
| cmpq rbx(rbp), 0 ; Mul64/IMul64
| | je @error
| | movq rax, rbx(rbp)
| ; TODO: compare .0
| movss xmm0, rbx(rbp) ; Mulf32
| movsd xmm0, rbx(rbp) ; Mulf64
movl ebx, 0(BC)
andl ebx, 0xfff
| divl eax, rbx(rbp) ; Mul32
| divq rax, rbx(rbp) ; Mul64
| idivl eax, rbx(rbp) ; IMul32
| idivq rax, rbx(rbp) ; IMul64
| divss xmm0, rbx(rbp) ; Mulf32
| divsd xmm0, rbx(rbp) ; Mulf64
JUMP_NEXT_BC()

error:
PANIC(ERROR_DIVIDE_BY_ZERO, "divide by zero!")
```

* Test Compare Two Stack Values to ACC
    * 作用：比较栈中两个值，结果放入ACC中(`true`/`false`)
    * 类型：`AB`型
        * `参数A`：操作数在栈中的偏移量
        * `参数B`：操作数在栈中的偏移量
    * 副作用：写入`ACC`

```asm
; [ TestEqual32/64/Ptr/f32/f64 ]
; [ TestNotEqual32/64/Ptr/f32/f64 ]
; [ TestLessThan32/64/f32/f64 ]
; [ TestLessThanOrEqual32/64/f32/f64 ]
; [ TestGreaterThan32/64/f32/f64 ]
; [ TestGreaterThanOrEqual32/64/f32/f64 ]
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl ebx, 12
negq rbx
| movl eax, rbx(rbp) ; 32
| movq rax, rbx(rbp) ; 64/Ptr
| movss xmm0, rbx(rbp) ; f32
| movsd xmm0, rbx(rbp) ; f64
movl ebx, 0(BC)
andl ebx, 0xfff
negq rbx
| cmpl eax, rbx(rbp) ; 32
| cmpq rax, rbx(rbp) ; 64/Ptr
| comiss xmm0, rbx(rbp) ; f32
| comisd xmm0, rbx(rbp) ; f64
nop
| je @true ; TestEqual32/64/Ptr/f32/f64
| jne @true ; TestNotEqual32/64/Ptr/f32/f64
| jl @true ; TestLessThan32/64/f32/f64
| jle @true ; TestLessThanOrEqual32/64/f32/f64
| jg @true ; TestGreaterThan32/64/f32/f64
| jge @true ; TestGreaterThanOrEqual32/64/f32/f64
false:
xorq rax, rax ; set rax = 0
jmp near @done
true:
movq rax, 1
done:
JUMP_NEXT_BC()
```

* Test Compare Two String Values to ACC
    * 作用：比较栈中两个字符串，结果放入ACC中(`true`/`false`)
    * 类型：`AB`型
        * `参数A`：操作数在栈中的偏移量
        * `参数B`：操作数在栈中的偏移量
    * 副作用：写入`ACC`

```asm
; [ TestStringEqual TestStringNotEqual ]
movl ebx, 0(BC)
andl ebx, 0xfff000
shrl ebx, 12
negq rbx
movq r8, rbx(rbp) ; r8 = string1

movl ebx, 0(BC)
andl ebx, 0xfff
negq rbx
cmpq r8, rbx(rbp) ; if (string1 == string2) then equals
je near @equals
movq r9, rbx(rbp) ; r9 = string2

movl ebx, String_len(r8) ; ebx = string1.len
cmpl ebx, String_len(r9) ; if (string1.len == string2.len)
jne near @not_equals

movl ebx, 0xfffffff0 ; rbx &= -16
movl eax, 16 ; string1.len
movl edx, 16 ; string2.len

pcmpestr_loop:
cmpl ebx, 0
jle near @pcmpestr_remain
subl ebx, 16
movdqu xmm0, rbx*1+String_data(r8) ; move string1's 16 bytes to xmm0
pcmpestri xmm0, rbx*1+String_data(r9), EqualEach|NegativePolarity
jc near @not_equals
jmp near @pcmpestr_loop

pcmpestr_remain:
movl ebx, String_len(r8) ; ebx = string1.len
andl ebx, 0xf ; remain length
movl eax, ebx ; string1.len = remain length
movl edx, ebx ; string2.len = remain length
movl ebx, String_len(r8) ; ebx = string1.len
andl ebx, 0xfffffff0 ; ebx &= -16
movdqu xmm0, rbx*1+String_data(r8) ; move string1's 16 bytes to xmm0
pcmpestri xmm0, rbx*1+String_data(r9), EqualEach|NegativePolarity
jc near @not_equals

not_equals:
| xorq rax, rax ; TestStringEqual
| | jmp near @done
| movq rax, 1   ; TestStringNotEqual
| | jmp near @done
equals:
| xorq rax, rax ; TestStringNotEqual
| movq rax, 1   ; TestStringEqual
done:
JUMP_NEXT_BC()
```



* Truncate integer number
    * 作用：将一个整数类型截断到指定大小，常用于整数从大到小类型转换
    * 类型：`A`型
        * `参数A`：源数的栈偏移量
    * 副作用：写入`ACC`

```asm
; [ Truncate32To8/16 Truncate64To32 ]
movl ebx, 0(BC)
andl ebx, 0xffffff
negq rbx
| movl eax, rbx(rbp) ; Truncate32To8
| | andl eax, 0xff
| movl eax, rbx(rbp) ; Truncate32To16
| | andl eax, 0xffff
| movl eax, rbx(rbp) ; Truncate64To32
JUMP_NEXT_BC()
```

* Zero-Extend integer number
    * 作用：将一个整数0扩展到指定大小，常用于无符号整数从小到大转换
    * 类型：`A`型
        * `参数A`：源数的栈偏移量
    * 副作用：写入`ACC`

```asm
; [ ZeroExtend8/16To32 ZeroExtend32To64 ]
movl ebx, 0(BC)
andl ebx, 0xffffff
negq rbx
| movzxb eax, rbx(rbp) ; ZeroExtend8To32
| movzxw eax, rbx(rbp) ; ZeroExtend16To32
| movl eax, rbx(rbp) ; ZeroExtend32To64
JUMP_NEXT_BC()
```

* Sign-Extend integer number
  * 作用：将一个整数符号扩展到指定大小，常用于有符号整数从小到大转换
  * 类型：`A`型
  * `参数A`：源数的栈偏移量
  * 副作用：写入`ACC`

```asm
; [ SignExtend8/16To32 SignExtend32To64 ]
movl ebx, 0(BC)
andl ebx, 0xffffff
negq rbx
| movsxb eax, rbx(rbp) ; SignExtend8To32
| movsxw eax, rbx(rbp) ; SignExtend16To32
| movsxd rax, rbx(rbp) ; SignExtend32To64
JUMP_NEXT_BC()
```

* Cast number to 32 bits floating-number
    * 作用：将一个数值类型转换为32bit浮点数
    * 类型：`A`型
        * `参数A`：源数的栈偏移量
    * 副作用：写入`XACC`

```asm
; [ I32/U32/I64/U64/F64ToF32 ]
movl ebx, 0(BC)
andl ebx, 0xffffff
negq rbx
; Convert Dword Integer to Scalar Single-Precision FP Value
| cvtsi2ssl xmm0, rbx(rbp) ; I32/U32ToF32
; Convert Scalar Single-Precision FP Value to Scalar Double-Precision FP Value
| cvtsi2ssq xmm0, rbx(rbp) ; I64/U64ToF32
; Convert Scalar Double-Precision FP Value to Scalar Single-Precision FP Value
| cvtsd2ss xmm0, rbx(rbp) ; F64ToF32
JUMP_NEXT_BC()
```

* Cast number to 64 bits floating-number
    * 作用：将一个数值类型转换为64bit浮点数
    * 类型：`A`型
        * `参数A`：源数的栈偏移量
    * 副作用：写入`XACC`

```asm
; [ I32/U32/I64/U64/F32ToF64 ]

movl ebx, 0(BC)
andl ebx, 0xffffff
negq rbx
; Convert Dword Integer to Scalar Single-Precision FP Value
| cvtsi2sdl xmm0, rbx(rbp) ; I32/U32ToF64
; Convert Scalar Single-Precision FP Value to Scalar Double-Precision FP Value
| cvtsi2sdq xmm0, rbx(rbp) ; I64/U64ToF64
; Convert Scalar Double-Precision FP Value to Scalar Single-Precision FP Value
| cvtss2sd xmm0, rbx(rbp) ; F32ToF64
JUMP_NEXT_BC()
```

* Call Bytecode Function
    * 作用：调用一个函数，函数对象指针放在`rax`中
    * 类型：`A`型
        * `参数A`：参数字节数大小
    * 副作用：写入`ACC`或`XACC`

```asm
; [ CallBytecodeFunction ]

movl ebx, 0(BC)
andl ebx, 0xffffff
addl ebx, 15  ; ebx = ebx + 16 - 1
andl ebx, 0xfffffff0 ; ebx &= -16
subq rsp, rbx ; adjust sp to aligment of 16 bits(2 bytes)

; call bytecode function
movq Argv_0, rax
call InterpreterPump

; restore caller's BC register
movq SCRATCH, stack_frame_bytecode_array(rbp)
movl ebx, stack_frame_pc(rbp)
; [SCRATCH + rcx * 4 + BytecodeArray_instructions]
leaq BC, rbx*4+BytecodeArray_instructions(SCRATCH) 

movl ebx, 0(BC)
andl ebx, 0xffffff
addl ebx, 15  ; ebx = ebx + 16 - 1
andl ebx, 0xfffffff0 ; ebx &= -16
addq rsp, rbx ; recover sp

JUMP_NEXT_BC()
```

* Call Native Function
    * 作用：调用一个`native`修饰函数，函数对象指针放在`rax`中
    * 类型：`A`型
        * `参数A`：参数字节数大小
    * 副作用：写入`ACC`或`XACC`

```asm
; [ CallNativeFunction ]
movl ebx, 0(BC)
andl ebx, 0xffffff
; (x + m - 1) & -m
addl ebx, 15  ; ebx = ebx + 16 - 1
andl ebx, 0xfffffff0 ; ebx &= -16
subq rsp, rbx ; adjust sp to aligment of 16 bits(2 bytes)

movq SCRATCH, Closure_cxx_fn(rax)
leaq SCRATCH, Code_instructions(SCRATCH)
callq SCRATCH

movl ebx, 0(BC)
andl ebx, 0xffffff
addl ebx, 15  ; ebx = ebx + 16 - 1
andl ebx, 0xfffffff0 ; ebx &= -16
addq rsp, rbx ; recover sp

; test if throw exception in native function
; Can not throw c++ exception in native function!
cmpq Coroutine_exception(CO), 0
jne @throw
JUMP_NEXT_BC()

throw:
movq rax, Coroutine_exception(CO)
THROW()
```

> 一些复杂操作通过调用内部函数实现，而不独立实现bytecode了

```c++
// 管道发送
u32_t Runtime::ChannelSend32(Channel *chan, u32_t data);
u32_t Runtime::ChannelSend64(Channel *chan, u64_t data);

// 管道接收
u32_t Runtime::ChannelRecv32(Channel *chan);
u64_t Runtime::ChannelRecv64(Channel *chan);
f32_t Runtime::ChannelRecvf32(Channel *chan);
f64_t Runtime::ChannelRecvf64(Channel *chan);

// 创建内建对象
Any *Runtime::NewBuiltinObject(u8_t code, Address argv, Address end);
```

* Create a builtin object
    * 作用：创建一个内建对象，例如`array`,` map`,`channel` ... 
    * 类型：`FA`型
        * `参数F`：对象类型代码
        * `参数A`：参数字节数大小
    * 副作用：写入`ACC`

```asm
; [ NewBuiltinObject ]

movl ebx, 0(BC)
andl ebx, 0xff0000
shrl ebx, 16 ; param: F
movl Argv_0, ebx

movl ebx, 0(BC)
andl ebx, 0xffff ; param A
movq Argv_1, rsp
addq rbx, rsp
movq Argv_2, rbx

; Any *NewBuiltinObject(int code, Address argv, Address end)
movq r11, Runtime::NewBuiltinObject ; NewBuiltinObject(builtin_obj_code, argv, end)
call SwitchSystemStackCall
JUMP_NEXT_BC()

```

* Calling Return
    * 作用：从字节码函数中返回
    * 类型：`N`型
    * 副作用：无

```asm
; [ Return ]
; Keep rax, it's return value
movq SCRATCH, stack_frame_callee(rbp)
movq SCRATCH, Closure_mai_fn(SCRATCH)
; has exception handlers ?
test Function_tags(SCRATCH), Function::kExceptionHandleBit 
jz @done

; Uninstall caught handle
movq SCRATCH, Coroutine_caught(CO)
movq SCRATCH, CaughtNode_next(SCRATCH)
movq Coroutine_caught(CO), SCRATCH ; coroutine.caught = coroutine.caught.next

done:
movq SCRATCH, stack_frame_callee(rbp)
movq SCRATCH, Closure_mai_fn(SCRATCH)
addq rsp, Function_stack_size(SCRATCH) ; recover stack
popq rbp
ret

```

* Yield
    * 作用：主动让出控制权，让其他协程得到调度机会。
    * 类型：`A`型
    * `参数A` ：控制代码
        * `YIELD_FORCE` 强制让出控制权
        * `YIELD_PROPOSE` 建议让出
        * `YIELD_RANDOM` 随机1/8几率
    * 副作用：无

```asm
; [ Yield ]
movl ebx, 0(BC)
andl ebx, 0xffffff
cmpl ebx, YIELD_FORCE
je @force
cmpl ebx, YIELD_PROPOSE
je @propose
cmpl ebx, YIELD_RANDOM
je @random
; bad bytecode
PANIC(ERROR_BAD_BYTE_CODE, "incorrect yield control code.")

propose:
cmpl Coroutine_yield(CO), 0
jg @force
jmp near @done

random:
rdrand ebx
andl ebx, 0xfffffff0
cmpl ebx, 0
je @force
jmp near @done

; save acc, xacc, rsp, rbp, pc then jump to resume point
force:
movq Coroutine_acc(CO), rax
movsd Coroutine_xacc(CO), xmm0
movl eax, stack_frame_pc(rbp)
movq Coroutine_pc(CO), rax
movq Coroutine_bp(CO), rbp
movq Coroutine_sp(CO), rsp
movq rbx, Coroutine_sys_pc(CO) ; rbx = suspend point
jmp far rbx ; jump to suspend point

done:
JUMP_NEXT_BC()

```

* Throw a Exception
    * 作用：抛出ACC中的异常对象。
    * 类型：`N`型
    * 副作用：无

```asm
; [ Throw ]
movq SCRATCH, Coroutine_caught(CO)
movq rbx, CaughtNode_next(SCRATCH)
movq Coroutine_caught(CO), rbx ; coroutine.caught = coroutine.caught.next
movq rbx, CaughtNode_pc(SCRATCH)
jmp far rbx ; throw again to prev handler
```

* Goto absolute position
    * 作用：跳转到绝对位置。
    * 类型：`A`型
        * `参数A` ：绝对PC位置
    * 副作用：无

```asm
; [ Goto ]
movl ebx, 0(BC)
andl ebx, 0xffffff
movl stack_frame_pc(rbp), ebx
START_BC()
```

* Condition Goto by ACC Boolean Value
    * 作用：依赖ACC中的布尔值进行跳转
    * 类型：`A`型
        * `参数A`：PC绝对值，如果满足条件，直接跳转到这个位置
    * 副作用：写入`ACC`

```asm
; [ GotoIfTrue GotoIfFalse ]
movl ebx, 0(BC)
andl ebx, 0xffffff
coml rax, 0
| je @next ; GotoIfTrue
| jne @next ; GotoIfFalse
movl stack_frame_pc(rbp), ebx
START_BC()
next:
JUMP_NEXT_BC()
```

* Jump forward relatively position
    * 作用：前进跳转到相对位置。
    * 类型：`A`型
        * `参数A` ：PC增量
    * 副作用：无

```asm
; [ JumpForward ]
movl ebx, 0(BC)
andl ebx, 0xffffff
addl stack_frame_pc(rbp), ebx
START_BC()
```

* Jump backward relatively position
    * 作用：后退跳转到相对位置。
    * 类型：`A`型
        * `参数A` ：PC减量
    * 副作用：无

```asm
; [ JumpBackward ]
movl ebx, 0(BC)
andl ebx, 0xffffff
subl stack_frame_pc(rbp), ebx
START_BC()
```

