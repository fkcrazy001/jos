
../build/kernel/task.o:     file format elf32-i386


Disassembly of section .text:

00000000 <running_task>:
   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	89 e0                	mov    %esp,%eax
   5:	25 00 f0 ff ff       	and    $0xfffff000,%eax
   a:	90                   	nop
   b:	5d                   	pop    %ebp
   c:	c3                   	ret

0000000d <yield>:
   d:	55                   	push   %ebp
   e:	89 e5                	mov    %esp,%ebp
  10:	83 ec 18             	sub    $0x18,%esp
  13:	e8 fc ff ff ff       	call   14 <yield+0x7>
  18:	89 c2                	mov    %eax,%edx
  1a:	a1 00 00 00 00       	mov    0x0,%eax
  1f:	39 c2                	cmp    %eax,%edx
  21:	75 07                	jne    2a <yield+0x1d>
  23:	a1 00 00 00 00       	mov    0x0,%eax
  28:	eb 05                	jmp    2f <yield+0x22>
  2a:	a1 00 00 00 00       	mov    0x0,%eax
  2f:	89 45 f4             	mov    %eax,-0xc(%ebp)
  32:	83 ec 0c             	sub    $0xc,%esp
  35:	ff 75 f4             	push   -0xc(%ebp)
  38:	e8 fc ff ff ff       	call   39 <yield+0x2c>
  3d:	83 c4 10             	add    $0x10,%esp
  40:	90                   	nop
  41:	c9                   	leave
  42:	c3                   	ret

00000043 <task_create>:
  43:	55                   	push   %ebp
  44:	89 e5                	mov    %esp,%ebp
  46:	83 ec 10             	sub    $0x10,%esp
  49:	8b 45 08             	mov    0x8(%ebp),%eax
  4c:	05 00 10 00 00       	add    $0x1000,%eax
  51:	89 45 fc             	mov    %eax,-0x4(%ebp)
  54:	83 6d fc 14          	subl   $0x14,-0x4(%ebp)
  58:	8b 45 fc             	mov    -0x4(%ebp),%eax
  5b:	89 45 f8             	mov    %eax,-0x8(%ebp)
  5e:	8b 45 f8             	mov    -0x8(%ebp),%eax
  61:	c7 40 08 11 11 11 11 	movl   $0x11111111,0x8(%eax)
  68:	8b 45 f8             	mov    -0x8(%ebp),%eax
  6b:	c7 40 04 22 22 22 22 	movl   $0x22222222,0x4(%eax)
  72:	8b 45 f8             	mov    -0x8(%ebp),%eax
  75:	c7 00 33 33 33 33    	movl   $0x33333333,(%eax)
  7b:	ba 43 00 00 00       	mov    $0x43,%edx
  80:	8b 45 f8             	mov    -0x8(%ebp),%eax
  83:	89 50 0c             	mov    %edx,0xc(%eax)
  86:	8b 45 f8             	mov    -0x8(%ebp),%eax
  89:	8b 55 0c             	mov    0xc(%ebp),%edx
  8c:	89 50 10             	mov    %edx,0x10(%eax)
  8f:	8b 55 fc             	mov    -0x4(%ebp),%edx
  92:	8b 45 08             	mov    0x8(%ebp),%eax
  95:	89 10                	mov    %edx,(%eax)
  97:	90                   	nop
  98:	c9                   	leave
  99:	c3                   	ret

0000009a <func1>:
  9a:	55                   	push   %ebp
  9b:	89 e5                	mov    %esp,%ebp
  9d:	83 ec 08             	sub    $0x8,%esp
  a0:	fb                   	sti
  a1:	83 ec 08             	sub    $0x8,%esp
  a4:	68 04 00 00 00       	push   $0x4
  a9:	68 00 00 00 00       	push   $0x0
  ae:	e8 fc ff ff ff       	call   af <func1+0x15>
  b3:	83 c4 10             	add    $0x10,%esp
  b6:	eb e9                	jmp    a1 <func1+0x7>

000000b8 <func2>:
  b8:	55                   	push   %ebp
  b9:	89 e5                	mov    %esp,%ebp
  bb:	83 ec 08             	sub    $0x8,%esp
  be:	66 87 db             	xchg   %bx,%bx
  c1:	fb                   	sti
  c2:	83 ec 08             	sub    $0x8,%esp
  c5:	68 0c 00 00 00       	push   $0xc
  ca:	68 00 00 00 00       	push   $0x0
  cf:	e8 fc ff ff ff       	call   d0 <func2+0x18>
  d4:	83 c4 10             	add    $0x10,%esp
  d7:	eb e9                	jmp    c2 <func2+0xa>

000000d9 <task_init>:
  d9:	55                   	push   %ebp
  da:	89 e5                	mov    %esp,%ebp
  dc:	83 ec 08             	sub    $0x8,%esp
  df:	a1 00 00 00 00       	mov    0x0,%eax
  e4:	68 9a 00 00 00       	push   $0x9a
  e9:	50                   	push   %eax
  ea:	e8 54 ff ff ff       	call   43 <task_create>
  ef:	83 c4 08             	add    $0x8,%esp
  f2:	a1 00 00 00 00       	mov    0x0,%eax
  f7:	68 b8 00 00 00       	push   $0xb8
  fc:	50                   	push   %eax
  fd:	e8 41 ff ff ff       	call   43 <task_create>
 102:	83 c4 08             	add    $0x8,%esp
 105:	e8 fc ff ff ff       	call   106 <task_init+0x2d>
 10a:	90                   	nop
 10b:	c9                   	leave
 10c:	c3                   	ret
