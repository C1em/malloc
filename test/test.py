from subprocess import Popen, PIPE

p = Popen("./a.out", stdout=PIPE)
output = p.communicate()[0]

while p.returncode == 0:
	p = Popen("./a.out", stdout=PIPE)
	output = p.communicate()[0]

	print(p.returncode)

print(output)
