# shell_trx

shell_trx is a small program used to generate an [expect](http://expect.sourceforge.net/) script.
The script mainly contains shell commands 'echo' that allows to write a binary file.
It could be used for example with telnet (or ssh...), to transfer a file.
The only requirement is that 'echo' command must implements flags -e and -n.

## History

A few times ago, i bought a small router.
It was a ZTE MF10, a small device based on Ralink RT 3050 SoC.
And i wanted to install [OpenWrt](http://www.openwrt.org) on it.

Posts in this [forum](http://foro.seguridadwireless.net/openwrt/flashear-xte-mf10/) confirmed that it was possible, but it required to open the router and to solder some wires in order to access the router via its serial port.
As i don't have any soldering skill, i searched for another solution.

Always in the same forum, someone discovered that the router can be accessed with telnet on port 4719.

I connected to my router to look for ways to transfer a file on it (the OpenWrt firmware) and found nothing at all : neither netcat, wget, tftp or ftp client. Nothing !!!

But fortunately, the shell embedded in the device (the version of ash provided by busybox) implemented the echo command with flags -n and -e.
That's was enough to transfer a binary file.

I just have to generate an expect script containing the 'echo' commands containing the bytes to transfer.
Of course writing by hand 'echo' command is very tedious if the size of the file to write exceeds a few bytes.
So i wrote this small program : it takes a file on input, reads it and generates the expect script.

## Compilation

Just launch the command ```make``` in the directory containing the Makefile.

## Usage

Of course you must have the expect utility installed on your machine.

The programs accepts several parameters but just a few of them are mandatory.

- **in** : the name of file to encode with the 'echo' commands. This parameter is mandatory.

- **out** : the name of the file that will be written with the 'echo' commands. This parameter is optional. Default value is '/tmp/fil'.

- **script** : the name of the script containing the expect commands. This parameter is optionnal. Default value is '/tmp/shell_trx'.

- **shell** : the name of the program that expect must launch. This parameter is optionnal. Default value is 'telnet'.

- **interactive_before** : value of this optional parameter could be 0 or 1. Default value is 1.
A value of 1 instructs shell_trx to generate in the expect script, the 'interactive' command, just after the 'spawn' command that launch the shell.
It allows the user to enter shell commands before the file transfer starts.
For example with telnet it allows to enter login and password, and to set PS1 environment variable to adjust the prompt.
To exit interactive mode, type CTRL+C.

- **interactive_after** : value of this optional parameter could be 0 or 1. Default value is 1.
A value of 1 instructs the program to generate in the expect script, the 'interactive' command, after the file transfer.
To exit interactive mode, type CTRL+C.

- **prompt** : this very important parameter is mandatory.
It's value is a regular expression used by the 'expect_before' command in the expect script.
The value of this parameter is what allows expect to send each 'echo' command at the right time.
If the value is wrong either there will be a timeout because expect does not receive the prompt. This type of error will be detected and the script will exit.
Or expect will send 'echo' commands too fast and the file written will have the wrong size. But this time there will be no detection of this type of error.
I strongly advise to follow hints from expect documentation. IMHO the most important is to give a pattern that include the end of what-ever you expect to see.
Another important point is to have a prompt that does not occurs in the bytes of the file transferred.

When the expect script is generated, it could eventually be modified before being executed with command ```expect -b script_name```

## Example

To transfer in my router a version of Busybox i compiled that includes a tftp client and md5sum utility, i used the following command to generate the expect script.

```shell_trx in=/tmp/busybox out=/var/busybox script=/tmp/shell_trx prompt='zte-mf10 $' shell='telnet 192.168.0.1 4719'```

The expect script looks like that

```
spawn telnet 192.168.0.1 4719
exp_send_user "**** Shell launched. Entering interactive mode. Type CTRL+C to exit interactive mode and transfer the file ****\n"
interact \003 return
set timeout 10
expect_before -re {zte-mf10 $}
expect_after {
  timeout {
    exp_send_user "Timeout. Exiting\n"
    exit
  }
  eof {
    exp_send_user "EOF. Exiting\n"
    exit
  }
}
exp_send -- {stty -echo}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n '\x7fELF\x01\x01\x01\x00\x01\x00\x00\x00\x00\x00\x00\x00\x02\x00\x08\x00\x01\x00\x00\x00`\x01@\x004\x00\x00\x00\x1cs\x0a\x00\x05\x10\x00p4\x00 \x00\x04\x00(\x00\x1f\x00\x1c\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00@\x00' > '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n '\x00\x00@\x00\xf8\x86\x09\x00\xf8\x86\x09\x00\x05\x00\x00\x00\x00\x00\x01\x00\x01\x00\x00\x00\xf8\x86\x09\x00\xf8\x86J\x00\xf8\x86J\x00\xa8\x0a\x00\x00\x9cf\x01\x00\x06\x00\x00\x00\x00\x00\x01\x00\x07\x00\x00\x00\xf8\x86\x09\x00\xf8\x86J\x00' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n '\xf8\x86J\x00\x04\x00\x00\x00\x0c\x00\x00\x00\x04\x00\x00\x00\x04\x00\x00\x00Q\xe5td\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07\x00\x00\x00\x04\x00\x00\x00\x0b\x00\x1c<\xac\x09\x9c\x27!\xe0\x99\x03' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n '\xe0\xff\xbd\x27\x10\x00\xbc\xaf\x1c\x00\xbf\xaf\x18\x00\xbc\xaf\x1c\x00\xbf\x8f\x08\x00\xe0\x03 \x00\xbd\x27\x01\x00\x11\x04\x00\x00\x00\x00\x98\x00\x10\x0c\x00\x00\x00\x00\x01\x00\x11\x04\x00\x00\x00\x00\xb0\x0d\x10\x0c\x00\x00\x00\x00\x00\x00\x00\x00' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n '@\x00\x19<\x82\x0d\x10\x08\x0869\x27\x00\x00\x00\x00@\x00\x19<\xa2\x06\x10\x08\x88\x1a9\x27\x00\x00\x00\x00@\x00\x19<\x0a\x0d\x10\x08(49\x27\x00\x00\x00\x00@\x00\x19<F\x0d\x10\x08\x1859\x27\x00\x00\x00\x00' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n '@\x00\x19<\xc3\x06\x10\x08\x0c\x1b9\x27\x00\x00\x00\x00@\x00\x19<\x96\x0c\x10\x08X29\x27\x00\x00\x00\x00K\x00\x1c<`\x0a\x9c\x27!\xf8\x00\x00@\x00\x04<D<\x84$\x00\x00\xa5\x8f\x04\x00\xa6\x27\xf8\xff\x01$' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n '$\xe8\xa1\x03\xe0\xff\xbd\x27@\x00\x07<\xb4\x00\xe7$I\x00\x08<4\x81\x08%\x10\x00\xa8\xaf\x14\x00\xa2\xaf\xfb\x04\x12\x0c\x18\x00\xbd\xaf\xff\xff\x00\x10\x00\x00\x00\x00\xd0\xff\xbd\x27(\x00\xb3\xafK\x00\x13<0\x92b\x92' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
.....
exp_send -- {echo -e -n 'ptions\x00__GI_strlen\x00__GI_fstat64\x00__pack_d\x00__resp\x00vfprintf\x00bb_pars' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n 'e_mode\x00__progname_full\x00setup_unzip_on_fd\x00strpbrk\x00tcsetpgrp\x00raise' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n '\x00is_well_formed_var_name\x00free\x00dealloc_bunzip\x00halt_main\x00netstat_m' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n 'ain\x00interface_opt_a\x00bb_do_delay\x00sigprocmask\x00__fputc_unlocked\x00get' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {echo -e -n 'sockname\x00get_ug_id\x00fopen64\x00ftruncate64\x00' >> '/var/busybox'}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send -- {stty echo}; exp_send "\r"
exp_send_user -- {}; exp_send_user "\n"
expect
exp_send_user "Transfer successfully ended\n"
exp_send "exit\r"
close
```

Then i launch the script with
```expect -b /tmp/shell_trx```

Expect spawns telnet and enters interactive mode which allows me
- to enter my login, password.
- give the environment variable PS1, the value 'zte-mf10 ' (that match the value of prompt parameter above).
- to copy in directory /var an executable under the name busybox. This file will be overwritten, but that small trick allows me to set the executable right to the file written (the router lacks the command chmod).

Finally i typed CTRL+C to exit interactive mode and let expect send the 'echo' commands that will write the file '/var/busybox' on the router.

## Remarks

I stronlgy advise to verify the integrity of the file written. Check the size of course and if possible md5, sha1 or sha256 checksum.
