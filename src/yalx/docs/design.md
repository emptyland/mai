
# Yalx Design

> Yalx = Yet another language X

## Types

Value types
Reference Types

### Primitive Types

All primitive types are value type.

|*Name*|Bits|
|------|----|
| `bool` | 8 |
| `int` | 32  |
| `uint` | 32 |
| `i8` | 8 |
| `u8` | 8 |
| `i16` | 16 |
| `u16` | 16 |
| `i32` | 32 |
| `u32` | 32 |
| `i64` | 64 |
| `u64` | 64 |
| `f32` | 32 |
| `f64` | 64 |
| `char` | 32 |

### Array Types

`string`

Literal:

```
"string" '汉字' "😀==???" "$key=$value"
```

`T[N]` for example: `int[8]` `i8[32]` `u8[128]`

Literal:

```
{1, 2, 3} {"hello", "world", "demo"}
```

> Array can be value type or reference type

### Annotation

```
annotation Value {
    id: int = 1 // Default value
    name: string
}

@Value(name = "${a}")
var a: int = 0
```

### Struct and Class

Struct type is value type and class type is reference type.

`struct`

```kotlin
struct Foo(
    val id: int,
    val name: string
) {
    private var foo: string
    private var bar: f32

    override fun toString() = "${this.foo}:${this.bar}"
}

val name = "name"
val foo = Foo(1, name)
foo.foo = "foo"
foo.bar = 1.1
val copiedFoo = foo // Deep copying
```

`class`

```kotlin
class Foo(val id: int, val name: string)
val foo = Foo(1, "name")
val bar = foo // Reference copying
```

`object`

```kotlin
object Foo {
    val i = 0
    val j = 1
    val k = 2
}

println(Foo.i, Foo.j)

```

### Any

Then any type can be any value👍.


Definitions:

```kotlin

@Lang
class any {
    native fun hashCode(): u32
    native fun toString(): string
}

```

### Function

```kotlin

fun foo(a: int, b: int) = a + b
val functionObject = (a:int, b:int)->a+b
functionObject(1, 2)

fun bar(a: int, b: int): (int, string) {
    return a + b, "hello"
}

val (value, hint) = bar(1, 2)

```

### Generics

Function Generics:

```kotlin
fun foo<T>(a: T, b: T) = a + b

println(foo(1, 2))
println(foo(1.1, 2.2))
```

Class/Struct Generics

```kotlin
struct Foo<T>(
    val a: T,
    val b: int
)

val foo = Foo<string>("hello", 0)


class Map<K, V> {
    class Entry<K, V>(
        val key: K,
        val value: V
    ) {
        var next: Entry<K, V>
    }

    var entries: Entry<K, V>[] = empty
}


```








