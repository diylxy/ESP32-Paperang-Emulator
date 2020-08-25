from tkinter import *
from tkinter.messagebox import *
from tkinter.font import Font
from tkinter.scrolledtext import *

import tkinter.filedialog
from PIL import Image
import requests
import time
from array import array
import threading
import qrcode
import pygame

printerURL = 'http://192.168.1.109'
fontttf = "default.ttf"
width_k = 1.32225                   #图片拉伸长度，防止失真
root = Tk()
root.title("Printer")
root.geometry("400x400")
root.minsize(width=400, height=800)
useDither = IntVar()
firstIndent = IntVar()
secondIndent = IntVar()
autoRotate = IntVar()
############################
def size(address):
    global resizedImage
    textbox.insert(1.0,'[图像处理]正在处理图片: 调整大小\n')
    image = Image.open(address).convert('L')
    
    width = image.size[0]
    height = image.size[1]
    if(height > width and autoRotate.get()):
        image = image.transpose(Image.ROTATE_90)
        textbox.insert(1.0,'[图像处理]正在处理图片: 旋转90度 \n')
    width = image.size[0]
    height = image.size[1]
    n=height/384.0
    w=int(width/n)
    h=int(height/n)
    resizedImage = image.resize((int(w * width_k),int(h)))
    resizedImage = resizedImage.transpose(Image.FLIP_LEFT_RIGHT)

def dither(image):
    factor = 1
    if(factor < 1):
        factor = 1
    textbox.insert(1.0,'[图像处理]正在处理图片: Floyd-Steinberg抖动\n')
    width = image.size[0]
    height = image.size[1]
    pimgGray = image.load()
    for y in range(0, height-1):
        for x in range(1, width-1):
            oldPixel = pimgGray[x, y]
            newPixel = round(round(factor*oldPixel/255) * (255/factor))
            pimgGray[x, y] = int(newPixel)
            quantErr = oldPixel - newPixel
            pimgGray[x+1, y] += int((quantErr*7/16.0))
            pimgGray[x-1, y+1] += int((quantErr*3/16.0))
            pimgGray[x, y+1] += int((quantErr*5/16.0))
            pimgGray[x+1, y+1] += int((quantErr/16.0))

def blackwhite(image, black):
    textbox.insert(1.0,'[图像处理]正在处理图片: 黑白\n')
    width = image.size[0]
    height = image.size[1]
    pimgGray = image.load()
    for y in range(0, height):
        for x in range(0, width):
            if(pimgGray[x,y] > black):
                pimgGray[x,y] = 255
            else:
                pimgGray[x,y] = 0


def procImage(filename):
    global resizedImage
    textbox.insert(1.0,'[信息]正在准备图片\n')
    try:
        size(filename)
        if(useDither.get()):
            dither(resizedImage)
        if(textthreshold.get() == ''):
            textthreshold.insert(0, 127)
        blackwhite(resizedImage, int(textthreshold.get()))
    except Exception as e:
        textbox.insert(1.0,'[错误]无法处理图片\n')
        textbox.insert(1.0,'[错误原因]' + str(e) + '\n')
        return
    textbox.insert(1.0,'[信息]处理后图片尺寸: ' + str(resizedImage.size[0]) + ' x ' + str(resizedImage.size[1]) + '\n')
    resizedImage.save('out.png')

def printNow():
    global resizedImage
    textbox.insert(1.0,'[信息]转换为hex数据\n')
    buffer = array('B')
    tmpbuffer = 0
    pimgGray = resizedImage.load()
    width = resizedImage.size[0]
    for j in range(0,width):
        for k in range(0, 48):
            for l in range(0, 8):
                tmpbuffer = tmpbuffer << 1
                if(pimgGray[j,k*8+l] == 0):
                    tmpbuffer = tmpbuffer + 1
            buffer.append(tmpbuffer & 0xff)
            tmpbuffer = 0
    fout = open('image.raw', 'wb')
    fout.write(buffer)
    fout.close()
    files = {'upload_file': open('image.raw', 'rb')}
    textbox.insert(1.0,'[信息]正在上传...\n')
    textbox.insert(1.0,'[信息]打印机URL：'+ printerURL +'\n')
    try:
        requests.post(printerURL + '/upload', files = files)
    except:
        textbox.insert(1.0,'[错误]上传失败\n')
        return;
    textbox.insert(1.0,'[信息]上传完成, 开始打印\n')

def printButton():
    t1 = threading.Thread(target=printNow)
    t1.setDaemon(True)
    t1.start()
    return


def procButton():
    t2 = threading.Thread(target=procImage, args=(filename,))
    t2.setDaemon(True)
    t2.start()
    return


def fileHandle():
    global filename
    filename = tkinter.filedialog.askopenfilename()
    if filename != '':
        textbox.insert(1.0,'[信息]选择了文件：' +filename+'\n')
        
        
def feedButton():
    t=threading.Thread(target=feedButtonThread)
    t.start()
    
def feedButtonThread():
    if(textFeed.get() != ''):
        requests.get(printerURL + '/lineFeed/' + textFeed.get())
        textbox.insert(1.0,'[信息]走纸命令发送完成, 行数:' + textFeed.get() +'\n')
        
def changeTextSize():
    pass

def isascii(s):
    """Check if the characters in string s are in ASCII, U+0-U+7F."""
    return len(s) == len(s.encode())

def printText():
    global resizedImage
    pygame.init()
    textstr = ''
    if(firstIndent.get()):
    	textstr = textstr + '    '
    textstr += edittext.get(1.0, END)
    wordwidth = int(textSize.get())
    outstr = ''
    
    if(wordwidth <= 8 or wordwidth > 384):
        textbox.insert(1.0,'[警告]点阵宽度自动修改为16\n')
        wordwidth=16
    
    split = int(384/wordwidth)
    textbox.insert(1.0,'[自动排版]单行最多可容纳中文字符' + str (split) + '个\n')
    tmp = list(textstr)
    j = 0
    for i in range(len(tmp)):
        if j > (384-wordwidth) and tmp[i] != '\n':
            outstr = outstr + '\n'+ str(tmp[i]) 
            j = 0
            if(secondIndent.get()):
            	outstr = outstr + '    '
            	j = 2*wordwidth
            continue
        elif tmp[i] == '\n':
            outstr = outstr + str(tmp[i])
            j = 0
            if(firstIndent.get()):
            	outstr = outstr + '    '
            	j=2*wordwidth
            continue
        else:
        	outstr = outstr + str(tmp[i])
        if(isascii(tmp[i])):
            j = j + wordwidth/2
        else:
        	j = j + wordwidth
    textbox.insert(1.0,'[自动排版]正在生成图片\n')
    font = pygame.font.Font(fontttf, wordwidth)
    strlist = outstr.splitlines()
    all_ftext = []
    sum_height = 0
    for text in strlist:
        sum_height = sum_height + font.get_height()
        ftext = font.render(text, False, (0, 0, 0), (255, 255, 255))
        all_ftext.append(ftext)

    textSurface = pygame.Surface((384, sum_height + 10))
    textSurface.fill((255,255,255))
    start_height = 5
    for ftext in all_ftext:
        textSurface.blit(ftext, (0, start_height))
        start_height = start_height + ftext.get_height()

    pygame.image.save(textSurface, "text.bmp")
    textbox.insert(1.0,'[自动排版]完成\n')
    
    resizedImage = Image.open("text.bmp").convert('L')
    resizedImage = resizedImage.transpose(Image.ROTATE_90)
    textbox.insert(1.0,'[图像处理]正在处理图片: 旋转90度 \n')
    resizedImage = resizedImage.transpose(Image.FLIP_LEFT_RIGHT)
    s1 = resizedImage.size
    resizedImage = resizedImage.resize((int(s1[0] *width_k), int(s1[1])))
    blackwhite(resizedImage, 127)
    printNow()
    
def printTextButton():
    t=threading.Thread(target=printText)
    t.setDaemon(True)
    t.start()

def addTextButton():
    filen = tkinter.filedialog.askopenfilename()
    if filen != '':
        try:
            file = open(filen, "r")
            edittext.insert(END,str(file.read()))
        except Exception as e:
        	textbox.insert(1.0, '[ERROR]追加文本错误，错误原因：' +str(e) + '\n')
def delTextButton():
	edittext.delete('1.0',  END)
def changeFont():
    global fontttf
    pickedfiletypes = (('ttf 文件', '*.ttf'), ('ttc 文件', '*.ttc'))
    tmpfont = tkinter.filedialog.askopenfilename(filetypes=pickedfiletypes)
    if(tmpfont != ''):
        fontttf = tmpfont
        
def splitline():
	textbox.insert(1.0,'[信息]正在打印分割线\n')
	buffer= array('B')
	for i in range(0, 24):
		buffer.append(0x00)
		buffer.append(0xff)
	fout = open('image.raw', 'wb')
	fout.write(buffer)
	fout.close()
	files = {'upload_file': open('image.raw', 'rb')}
	requests.post(printerURL + '/upload',files =files)
	textbox.insert(1.0,'[信息]分割线打印完成\n')
	
def lineButton():
	t=threading.Thread(target=splitline)
	t.setDaemon(True)
	t.start()
	
def qrButton():
	t=threading.Thread(target=printqrCode)
	t.setDaemon(True)
	t.start()
def printqrCode():
	qrimg = qrcode.make(textQR.get())
	pygame.init()
	font = pygame.font.Font(fontttf, int(textSize.get()))
	
	ftext = font.render(textQRName.get(), False, (0, 0, 0), (255, 255, 255))
	s1 = qrimg.size
	w = max(s1[0], ftext.get_width())
	h = s1[1]+ftext.get_height()
	pygame.image.save(ftext, "qr_tmp.bmp")
	desc = Image.open("qr_tmp.bmp")
	imgAll = Image.new('L', size=(w, h), color=255)
	imgAll.paste(qrimg, (0,0))
	if(w == ftext.get_width()):
		imgAll.paste(desc, (0, s1[1]))
	else:
		imgAll.paste(desc, (int((w - ftext.get_width())/2), s1[1]))
	imgAll.save("qr.png")
	textthreshold.delete(0, END)
	textthreshold.insert(0, '127')
	ckdither.deselect()
	procImage('qr.png')
	printNow()
###################
f0 = Frame(root)
btn = Button(f0,text="选择文件...",command=fileHandle)
btn.pack(side=LEFT)

btnproc = Button(f0,text="图像二值化",command=procButton)
btnproc.pack(side=LEFT)
f0.pack()

ckdither = Checkbutton(root, onvalue = 1, offvalue = 0, variable=useDither, text='使用Floyd-Steinberg抖动')
ckdither.pack()

ckrotate = Checkbutton(root, onvalue = 1, offvalue = 0, variable=autoRotate, text='自动旋转')
ckrotate.select()
ckrotate.pack()

fm1=Frame(root)
lb = Label(fm1,text = '阈值(0-255越小越亮)：')
lb.pack(side=LEFT)
textthreshold = Entry(fm1, width=10)
textthreshold.insert(0, '100')
textthreshold.pack(side=LEFT)
fm1.pack()

btnprint = Button(root,text="打印",command=printButton)
btnprint.pack()
fm2=Frame(root)
btnFeed = Button(fm2,text='走纸',command=feedButton)
btnFeed.pack(side=LEFT)

lb = Label(fm2,text = '行数:')
lb.pack(side=LEFT)

textFeed = Entry(fm2, width=10)
textFeed.insert(0, '20')
textFeed.pack(side=LEFT)

fm2.pack()
lb = Label(root,text = '输入文字:')
lb.pack()
edittext = ScrolledText(root, width=50, height=10)
edittext.pack()

fm3=Frame(root)
lb = Label(fm3,text = '中文点阵宽度：')
lb.pack(side=LEFT)
textSize = Entry(fm3, width=10)
textSize.insert(0, '20')
textSize.pack(side=LEFT)
fm3.pack()
ckfirstindent = Checkbutton(root, onvalue = 1, offvalue = 0, variable=firstIndent, text='首行缩进2字符')
ckfirstindent.pack()

cksecondindent = Checkbutton(root, onvalue = 1, offvalue = 0, variable=secondIndent, text='悬挂缩进2字符')
cksecondindent.pack()

f4 = Frame(root)
btnprint = Button(f4,text="字体...",command= changeFont)
btnprint.pack(side=LEFT)
btnprint = Button(f4,text="追加...",command=addTextButton)
btnprint.pack(side=LEFT)
btnprint = Button(f4,text="打印文字",command=printTextButton)
btnprint.pack(side=LEFT)
btnclear = Button(f4,text="清空区域",command=delTextButton)
btnclear.pack(side=LEFT)
f4.pack()
f5=Frame(root)
btnline = Button(f5,text="直接打印分割线",command=lineButton)
btnline.pack(side=LEFT)
btnqr = Button(f5,text="打印二维码",command=qrButton)
btnqr.pack(side=LEFT)
f5.pack()

f6 = Frame(root)
lb = Label(f6, text='二维码内容: ')
lb.pack(side=LEFT)
textQR = Entry(f6, width=15)
textQR.pack(side=LEFT)
f6.pack()

f7= Frame(root)
lb = Label(f7, text='二维码注释: ')
lb.pack(side=LEFT)
textQRName = Entry(f7, width=15)
textQRName.pack(side=LEFT)
textQRName.insert(0, 'Hello World')

f7.pack()
lb = Label(root,text = 'Log:')
lb.pack()
textbox = ScrolledText(root, width=50, height=8)
textbox.pack()

root.mainloop()