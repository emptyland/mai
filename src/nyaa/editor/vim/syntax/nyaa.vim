
syntax keyword nyaaContant nil true false
syntax keyword nyaaKeyword var val not def new do lambda property class object

syntax keyword nyaaFunc raise getmetatable setmetatable len str log print yield
syntax keyword nyaaFunc range pairs pcall assert require loadfile loadstring garbagecollect

syntax match nyaaInteger "\<\d\+\>"
syntax match nyaaInteger "\<\-\d\+\>"
syntax match nyaaInteger "\<\+\d\+\>"
syntax match nyaaFloat "\<\d\*\.\d\+\>"
syntax match nyaaHex "\<0x\x\+\>"
syntax match nyaaFix "FIX\|FIXME\|TODO\|XXX"
syntax match nyaaOperator "\^\|@"

syntax region nyaaComment start="//" skip="\\$" end="$"
syntax region nyaaString start=+L\="+ skip=+\\\\\|\\"+ end=+"+
syntax region nyaaRawString start=+L\='+ skip=+\\\\\|\\'+ end=+'+

syntax keyword nyaaCondition if else and or
syntax keyword nyaaLoop for while break continue in
syntax keyword nyaaReturn return

hi def link nyaaCondition Statement
hi def link nyaaLoop      Statement
hi def link nyaaReturn    Statement
hi def link nyaaFunc      Identifier
hi def link nyaaKeyword   Type

hi nyaaFix ctermfg=6 cterm=bold guifg=#0000FF
hi nyaaOperator ctermfg=1
hi nyaaInteger ctermfg=5
hi nyaaFloat ctermfg=5
hi nyaaHex ctermfg=5
hi nyaaString ctermfg=5
hi nyaaRawString ctermfg=1
hi nyaaComment ctermfg=2
hi nyaaContant ctermfg=5 guifg=#FFFF00
