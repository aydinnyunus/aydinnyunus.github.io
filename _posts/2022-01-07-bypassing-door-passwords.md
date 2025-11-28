---
layout: post
title: "Bypassing Door Passwords"
date: 2022-01-07
author: Yunus Aydın
---

Instead of a key, this type of lock system requires a numerical code to grant entry to a facility or property. The code is punched in by users via a numerical pad, similar to those on a basic calculator. If the correct code is entered, the door lock or deadbolt should release. Some mechanisms require batteries or a small electrical current to unlock.

Some keypad locks have an integrated security feature that keeps the door locked for a set amount of time (usually 10 to 15 minutes) after several incorrect attempts to enter the code.

The purpose of this research is to understand how insecure we live.

![Audio Smart Lock](https://miro.medium.com/v2/resize:fit:1000/format:webp/0*XEZo3-Sq09f2CK-O)

## Audio Smart Lock

Most Popular Entry Door Keypads in Turkey

- Perkotek
- ERD-1120
- Efes Digital Panel
- Mas
- AC 13PX
- Burg Wachter
- DIP40
- Lorex LR-DPH2
- M100
- MB05–03
- MB DYF40
- MLŞ 14–70
- MLŞ 14–107
- MRA 101
- Netalsan Obsidian
- ONDRIVE ED07
- OP705
- OP M400
- OP M500
- Pratik Kart
- Desi Steely
- Audio
- Teknoline
- Teknoline IMR18
- Desi UTOPIC
- WL02
- D45
- A20 Kapı Kilidi

## Most Used Default Passwords

- #0000
- 0000
- 0411#
- 0571
- #0789
- 0880#
- 1014
- 1111
- 1200#
- #1234
- **1234
- 1234
- 1234#
- 1357#
- 1453
- 1629#
- 1881
- 1979/
- *1992#
- 2000
- 2013
- 2020
- 2020#
- **2510
- 2707#
- 2828
- 3263
- 4050#
- 4233#
- 4570#
- 5555
- 5656#
- 5689#
- 5757
- 6161
- **6565
- **6771
- 7305
- **7788
- #7889
- **7890
- 8182#
- #8888#
- 88888888
- 8988#
- 9575
- 991453
- 991903
- 992211
- 992525
- 998877
- C12341234
- c2638
- C6161

## Real Life Tests

I try the admin password on "Audio Şifreli Kapı Kiliti" and it is successful. Now we can change all user's passwords, close the alarm, delete all users.

Also, we don't need to know the passwords, we can reset the admin password using some simple steps:

1. Push the reset button behind the box for 10 seconds.
2. Unplug the DT-8 socket and wait for 5 seconds and plug.
3. After 10 seconds passed, stop the pressing button and unplug the socket again at the same time.
4. After that, you can use default admin password :)

![Real Life Test](https://miro.medium.com/v2/resize:fit:1400/format:webp/0*mGGgCG6CaxKqC6WJ)

## Statistics

Until now, I try 3 Audio Door Locks and all of them have default passwords (including the admin password). You can contribute the statistics.

![Statistics](https://miro.medium.com/v2/resize:fit:1122/format:webp/0*v4sneCtbZPjIdYVD)

## Github Repository for Default Passwords

Enter the ID for getting information about the Model. Use the REST API with gateCracker tool.

![Models](https://miro.medium.com/v2/resize:fit:1140/format:webp/0*aRFq5D78zydmjtjZ)

## Source Code

- [https://github.com/aydinnyunus/gateCracker-REST](https://github.com/aydinnyunus/gateCracker-REST) - REST API
- [https://github.com/aydinnyunus/gateCracker](https://github.com/aydinnyunus/gateCracker) - Main tool (connect to REST API)

---

**Contact me:** [aydinnyunus@gmail.com](mailto:aydinnyunus@gmail.com)

