## Nyaa language design

### 1. Types

`int` : 63 bits integer

`float` : 63 bits float point number

`decimal` : [object] fixed point number

`string` : [object] binary strings

`table`: [object] map or array container

`object` : [object] user definition objects

`function` : [object] callable objects


```
var t = {
    name: 100,
    id: 0,
    0,
    1,
    [0]: 200
}
```

```
class bar(d, e) {
    property rw d, e
}
class foo(a,b,c) : bar(a,b) {
    
    property [rw] a, b, c
}

class foo: bar {
    def __init__ {
        m = 1, n = 1, o = 1, p = 1
        super:__init__(m, n)
    }
    def __apply__(a,b,c)
    def __add__(a,b,c)
    def doit = io.printf(‘%s’, m)
    def doit2(m, n) = (m_, n_) = (m, n)
    property [ro] m, n, o, p
    property [rw] a, b, c
}

```

#### 1.1 Primitive Types

int :

* max value: `4611686018427387903`
* min value: `-4611686018427387904`

```
var i = 1000
var i, j, k = 1, 2, 3
```

float : 

```
var i = 1.1
var i, j, k = 1.1, .1, 1.1e-10
```

#### 1.2 Object Types

decimal :

```
var i = @1.1
var i, j, k = @1.1, @.1, @1.1e-10
```

string :

```
var s = 'abcd'
var n = "abcd"
var m = `
	a
	b
	c
	d
`
var o = "\a\n\r\t\x00\u0102"
var p = "a=${p}"
```

table :

```
var t = {
	name: 'ok',
	age: 1
	inner: {
		id: 'okok',
	}
	[0] = 'first',
	[1] = 'second',
}
var a = {1,2,3,4,5} // array like
```

#### 1.3 User Definition Types

object :

```
object foo {
	def fn(a,b,c) = c, b, a
	def fn2(a,b) = a + b
	def setM(m) { m_ = m }
	
	propery[ro] m, n, o, p, q
	propery[rw] r, s, t = 1, 2, 3
}

print(foo.m, foo.n)
print(foo.fn(1,2,3))
print(foo.fn2(1,2))

```

class :

```
class foo(a, b, c) {
	property[ro] a, b, c
	
	def setA(self, a) { self.a_ = a }
	
}

var o = new foo(1, 2, 3)
print(o.a, o.b, o.c)

class bar(a, b, c, d, e, f) : foo(a, b, c) {
	def __init__(a, b, c, d, e, f) {
	}
	
	property[ro] d, e, f
}
o = new bar(1, 2, 3, 4, 5, 6)
print(o.d, o.e, o.f)


```

function :

```
def fn(a) = a + 1
def fn(t) = if (t) print (car(t)) else fn(cdr(t))
var t = map({1, 2, 3}, lambda (a) = a + 1)
```

### 2. Meta Table

| Fields | Explian | Remark |
| --- | --- | --- |
| `__init__` | constructor for new operator |  |
| `__index__` |  a[k] |   |
| `__newindex__` | a[k] = v  |   |
| `__call__` | a() |   |
| `__str__` | str(a) |   |
| `__add__` | a + b |   |
| `__sub__` | a - b |   |
| `__mul__` | a * b |   |
| `__div__` | a / b |   |
| `__mod__` | a % b |   |
| `__unm__` | ~a |   |
| `__concat__` | a..b |   |
| `__eq__` | a == b |   |
| `__lt__` | a < b |   |
| `__le__` | a <= b |   |

set / get meta table:

```
setmetatable(t, {})
getmetatable(t)
```

object and class's meta table:

```
class foo(a, b) {
	def setA(self, a) { self.a_ = a }
	property [ro] a, b
	property [rw] c, d
}

// metatable:

var foo_class = {
	__init__: lambda (o, a, b) { ... },
	__index__: lambda (o, f) { ... },
	__newindex__: lambda (o, f, v) { ... },
	
	__base__: nil, // no base class
	a: {kAOffset, ro},
	b: {kBOffset, ro},
	c: {kCOffset, rw},
	d: {kDoffset, rw},
	__total_size__: kSizeofFoo,
}
```

### 3. Syntax

### 4. Object Memory Map

Object-Pointer Map:

```
Primitive Value: 
                              1 bit
|<-----------63 bits--------->|   |
+-----------------------------+---+
|      Value-bits             | 1 |
+-----------------------------+---+

Object Reference: 
                              1 bit
|<-----------63 bits--------->|   |
+-----------------------------+---+
|      Native Pointer-bits    | 0 |
+-----------------------------+---+
```

Object Header:

```
|<---- 8 bytes ---->|
+-------------------+-------------
| Metatable Pointer | others ...
+-------------------+-------------
```