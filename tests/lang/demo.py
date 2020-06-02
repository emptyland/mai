#!/usr/bin/python

m = {}
for i in xrange(0, 100000):
    m[i] = i
    m[i] = "i=%d" % (i)
for i in xrange(0, 100000):
    m[i] == "i=%d" % (i)
for i in xrange(0, 100000):
    del m[i]

print(len(m))
