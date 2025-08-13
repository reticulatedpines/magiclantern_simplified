Introduction to Magic Lantern
=============================

Hello, potential camera hacker!  Have a Canon EOS camera and want to make it do extra things?  Wondering why Magic Lantern isn't on your camera yet?  Do you enjoy low-level messing around and exploring undocumented systems?  Or, you think you would and want to learn?  Give it a try, it's fun, if a little tricky sometimes.

Anyone who can help us keep things working and improving is greatly appreciated!

Building ML should work on Linux, Windows (WSL), or Mac (probably BSDs generally).

There are many possible ways to help out with the code, with different skills required.  We're pretty friendly to anyone that shows an interest and can help you learn any of these things, but it's not going to be quick:

  - Maintaining existing camera support (mostly C)
  - Adding support for new cameras (C, reverse engineering tools e.g. Ghidra / IDA Pro, sometimes ARM assembly)
  - Keeping things working / Magic Lantern infrastructure (make, python, qemu, automation skills)

The hard part is understanding how the camera works, and how to get it to do something else.  It's time consuming, often frustrating, but eventually satisfying: you can do things nobody has done before.

If you're not a coder and don't want to become one, you can still help: consider finding the old and new wiki links in the resource section, and improving documentation.

### New to programming?  New to git?  Etc
There's a lot more below, about how to keep things consistent and readable for ML code, but there's an overriding rule: we will help you learn.

So, don't worry too much - we will teach you, so you can skip reading it if you don't mind us telling you to change things later on.

### Important note
This repo is a fork.  However, the official repo is in practice unmaintained.  This repo's primary focus is getting ML working on newer cameras: Digic 6 and later, this begins with 5D mark IV, 750D, 200D era.  We also try to not break existing support.  Bug fixes or enhancements to old or new cams are welcome if there is reasonable confidence we don't break well supported cams.

# Important resources & contact info
Magic Lantern started in 2009, there's a lot of background that can be useful to understand.  This is messier than we'd like.  Some things are documented in the code, some on the forum, some in news groups, and some lost forever on IRC because there were no logs.

If you have problems getting a development environment working, asking in Discord will get you the quickest response.  The forums will also work.

The origin story!  
[https://trmm.net/Magic_Lantern_firmware/](https://trmm.net/Magic_Lantern_firmware/)

The official repo:  
[https://foss.heptapod.net/magic-lantern/magic-lantern](https://foss.heptapod.net/magic-lantern/magic-lantern)

Official forum, good for documenting things, or long running discussions that need to be referred to later.  
[https://www.magiclantern.fm/forum/](https://www.magiclantern.fm/forum/)

ML Discord, very useful for quick questions, or in-depth discussions.  Most dev chat happens here.  
[https://discord.gg/srY7GhQjYN](https://discord.gg/srY7GhQjYN)

Newsgroups, mostly defunct now, but can be valuable for reference at times.  
[https://groups.google.com/g/ml-devel](https://groups.google.com/g/ml-devel)

The old wiki, has more content, but is sometimes outdated:  
[https://magiclantern.fandom.com/wiki/Magic_Lantern_Firmware_Wiki](https://magiclantern.fandom.com/wiki/Magic_Lantern_Firmware_Wiki)

The new wiki, it's nicer...  but it's far from complete:  
[https://wiki.magiclantern.fm/](https://wiki.magiclantern.fm/)

# Building Magic Lantern
The build system is make, and has a lot of inherited complexity.  Most of this can be ignored.  Assuming you want to work on an individual cam, the 99D, and your machine has 16 cores / threads, this is all that is needed:

  - clone this repo
  - be in platform/99D
  - run: `make -j16 zip`
  - (possibly, fix your build environment because we don't check for pre-requisites, then repeat the previous step...)
  - unzip the result onto a prepared card

If using qemu for testing, the unzip step is not required, qemu run script will do this for you.

The build system is currently tested on Linux, but is expected to work on Windows (via WSL) and Mac. Try not to obviously break this.  If we have a volunteer to test on other systems we could make this guarantee stronger.

# Source control / repo conventions
We use git, currently hosted on Github, but we don't tie things strongly to Github.  Github specific features can be used if sensible, if the purpose also works locally (e.g., a test system could use Actions, but the same tests should be able to run locally too).

### Licensing
Magic Lantern is GPLv2 licensed.  Any contributed code or code changes must be licensed in a compatible way.  If in doubt, ask the maintainers of ML.

### Workflow
Getting a change to this repo looks like this:

  - fork the repo
  - branch from "dev", make your changes
  - make a PR against original repo

When you're ready for PR / merge to dev, you should first check dev hasn't had any changes since you branched: dev is updated via merge --ff-only.  If dev is updated before your PR is merged you are responsible for rebasing from dev before the PR can be accepted.

This gives us a very clean history on dev.  Conflicts are rare because pace of commits is low.

### Commits
Commit messages should explain why the change was made, not simply what change happened.  If it's a non trivial change, use a short (one line) and a long message (as much as you want) to give a good explanation.

E.g.:  
[https://github.com/reticulatedpines/magiclantern_simplified/commit/1d4363410031a7efb29e0aff24bc2cafd2a2dc1c](https://github.com/reticulatedpines/magiclantern_simplified/commit/1d4363410031a7efb29e0aff24bc2cafd2a2dc1c)

Most commits relate to a subarea.  Use a prefix on the message to indicate this.  E.g.:  
"5D4: fix misclassification of this cam as Dual Digic"  
"compositor: SX740 support"  
"gcc warnings: fix 5D2 bad intptr_t usage"

Give maintainers enough information so that they can understand the purpose and scope of the change and test correctness.

Commits should hold a self-contained unit of work.  This can cover a change in multiple files, but should not include multiple unrelated changes in one commit.  If working on a large feature, avoid single large commits where possible; build up the feature you're working on in pieces.

If you find a bug or mistake in your work, don't have one commit with the mistake and one undoing it: remove both commits before attempting PR.

### Code comments
Our code is not, and cannot, be self-documenting.  In many places the code depends on behaviour of the camera, which is not documented at all.  Do not be afraid of long comments when you are explaining things outside of the code.

Give enough information so that your code can be understood and tested by someone else.

E.g.:  
"Here we sleep for 50ms to give the mirror time to settle, without this the 99D errors with MechERR NG on UART".

### Making good changes
Changes intended for one cam must not break things for other cams.

It is okay if a change is visible for only those cameras it works on, and hidden for others.  It's better if it works on all cams.

Changes should be "safe enough".  Give people fair warning, and disable dangerous things by default. ML is inherently experimental, some risk is allowed.  Test to a sensible degree, relative to the changes being made.

Some examples:

  - Changes to ML GUI can be tested in Qemu and don't require tests on physical cams (still nice if you have the time!).
  - Changes involving interactions with camera hardware will sometimes emulate okay in Qemu but must also be tested on physical cameras, the emulation may not be representative.
  - Changes to ML bootloader must be tested extensively in Qemu first and on physical cams covering multiple Digic generations.  Problems here can be serious and leave the user with no useful feedback.

When working in risky areas, consider leaving your code disabled by default, with a flag or similar.  This means you can easily enable locally, but anyone accessing your public work will be safe by default.  Enable in the repo only after reasonable testing has occured.

The goal here is that someone finding this repo won't break their cam even if they don't talk to us first to get instructions.

Tests to run before commits:

  - "make zip" in platform dir must succeed: all enabled cams should build
  - no new compiler warnings

Tests to run before merges to dev:

  - as above, plus:
  - completed, user-accessible features should be tested extensively on a physical cam
  - (qemu tests when they exist)
