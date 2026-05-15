# Opening Golden Cheetah on ARM64 MacOS

Thank you for downloading Golden Cheetah for your arm64 Mac!

You should be able to drag and drop the `.app` file into the provided `/Applications` symlink.

However, when you try to open the application, you will likely be greeted by a warning along the lines of:

- Golden Cheetah can't be opened because it is from an unidentified developer
- Golden Cheetah can't be opened because the developer cannot be verified
- Golden Cheetah can't be opened because Apple cannot check it for malicious software
- Apple could not verify "GoldenCheetah.app" is free of malware that may harm your Mac or compromise your privacy

An application must be signed by an Apple-provided developer certificate to avoid these warnings; these cost $100 / year and the Golden Cheetah developers do not have one at this point.

Luckily these warnings can be worked around and should be a one-time-only nuisance.

To open the application:

- first drag and drop `GoldenCheetah.app` into the provided `/Applications` symlink
- once it is done copying, eject the dmg volume (either via the button in your Finder sidebar or the ⌘ -e keyboard shortcut)
- Open a Finder window to `/Applications` and find `GoldenCheetah.app`
- right click (or control-click) the app and choose `Open`
- you'll likely have to approve a security warning pop-up

To make this change permanent, after doing the above:

- open `System Settings`
- go to the `Privacy & Security` settings
- scroll down and find the `Open Anyway` button for Golden Cheetah

Fore more information, please review Apple's official guidance on this process: <https://support.apple.com/en-us/102445>
