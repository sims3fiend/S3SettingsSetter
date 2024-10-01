# S3SettingsSetter
New! Finally a release!

Was bored ~~ripping my hair out~~ trying to do optimizations so I made this instead of anything actually worthwhile. Rushing it out as I'm at work 3x12hrs starting tomorrow, so this is a **ALPHA IT SHOULD BE SAFE BUT IT MIGHT ALSO NOT BE**. Speedwriting this as I've gotta go sleepy.

What it does:

**Renderer Flags:**
This was the original idea and what this was solely going to be. I found a function that set things like wireframe and other renderer things, so I made a script for it.
Some useful things it can do are turning drop shadows off, which is nice for reshade as it lets you do a better looking AO. Some people might find the pips, scene boxes, etc. useful but idk.

**Game Values**:
Just the one for now, max lots. These are going to be static values that can be edited ingame. There's a few other ones I've found like ObjectSizeCullFactor but I need to properly test them. I'll also add some informational ones that can't be changed like loaded object count, etc.

**Game Config**:
This is where things went off the rails...
I'm hooking two functions here to grab and set boolean and string config values directly as they're called in memory, which is important as some are overridden or altered, or aren't listed in the files at all (ForceHighLODObjects my beloved). Currently it lists mostly everything in the SGR file and some ini settings. **Please keep in mind that some of these settings can be very damaging to your game and sanity, do not use ones you don't recognize on a save you love.**

I'll be adding tunables (like LightingCommon) into the list... eventually. Currently they're a little... weirdly/interestingly written in the decomp'd functions I've been looking at, so I haven't been able to get their values. There's probably a really simple way of doing all of this in one of the dlls or packages or something but idk.

I'll also probably try and find the in memory location of some of them, so they can be changed ingame without the need to restart


**Issues**:
If you have any problems/info please message me either on discord(fleshtexture) or tumblr ! Keep in mind it's still an ALPHA (or whatever) and I haven't really tested it that extensively.

Known problems:
- Config sorting is yucky
- Code looks yucky
