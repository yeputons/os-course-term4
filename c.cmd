@echo off
ssh -i "%HOME%\.ssh\id_rsa_b" yeputons@192.168.56.101 "cd os/os-course-term4 && make %*"
