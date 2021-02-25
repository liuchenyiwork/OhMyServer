#!/usr/bin/python
#coding:utf-8
import sys,os
import urllib
length = os.getenv('CONTENT_LENGTH')

if length:
    postdata = sys.stdin.read(int(length)) #从标准输入读取内容
    print "Content-type:text/html\n"
    print '<html>' 
    print '<head>' 
    print '<title>POST</title>' 
    print '</head>' 
    print '<body>' 
    print '<h2> Your POST data: </h2>'
    print '<ul>'
    for data in postdata.split('&'):    #把读取到的内容原封不动地打印回去
    	print  '<li>'+data+'</li>'
    print '</ul>'
    print '</body>'
    print '</html>'
    
else:
    print "Content-type:text/html\n"
    print 'no found'


