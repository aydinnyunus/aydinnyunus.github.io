---
layout: post
title: "Hacking Instagram Scammers"
date: 2022-04-11
author: Yunus Aydın
lang: en
description: "Security research on Instagram phishing scams. Learn how scammers steal Instagram accounts through phishing websites and how to investigate them using OSINT techniques and XSS vulnerabilities."
keywords: "instagram scammers, phishing, OSINT, XSS, security research, social engineering, cybersecurity, scam investigation"
canonical_url: "https://aydinnyunus.github.io/2022/04/11/hacking-instagram-scammers/"
---

# Hacking Instagram Scammers

## Summary

I see many phishing websites and messages these days. So I decided to make research how scammers scam people and stole hundreds of Instagram accounts. Also, these websites are shut down because probably they understand the abnormal activities.

## Finding Phishing Websites

When I am surfing on Twitter and Instagram, I saw tweets and stories about Copyright Violation messages, and nice to see that people are being aware of phishing (not all of them). So I started browsing these websites and understand how they stole credentials.

## Understanding Logic

I started on a website that you'll see on the first tweet. In one of his phishing messages, the scammer was sending a message to the Instagram user through the account named **Inhelptechnicanalyse**, stating that there was a copyright violation and demanding that the user visit [https://veriyfycontacsupports.com/](https://veriyfycontacsupports.com/) and fill out a form to verify their account.

After the Instagram user name was entered on the web page, the profile picture of the user was downloaded and displayed in the background, in order to increase the credibility of the page for which the password was requested.

**First Phishing Website** :

![Victim and Attacker become friends 🥰](https://miro.medium.com/v2/resize:fit:822/1*p6bto-Uwi3XSYcMqwGQgew.png)

Victim and Attacker become friends 🥰

> [https://twitter.com/bengujumping/status/1497926066496753671](https://twitter.com/bengujumping/status/1497926066496753671)

**Second Phishing Website :**

![](https://miro.medium.com/v2/resize:fit:838/1*4zJCOkaYXZlLTQKHEdfJ1Q.png)

Someone asks about cyber security [Ömer Çitak](https://twitter.com/om3rcitak), One year later, the same account sends him phishing mail 🙃

> [https://twitter.com/om3rcitak/status/1499341391427776520](https://twitter.com/om3rcitak/status/1499341391427776520)

After you enter your username and password, the website wants your email and password and then they can do everything on your account. I think all these processes are automated.

I use [ffuf](https://github.com/ffuf/ffuf) for directory brute force and use [SecLists](https://github.com/danielmiessler/SecLists/blob/master/Discovery/Web-Content/directory-list-2.3-medium.txt) for the wordlist. But I did not find any important directory. After that I back to the first page and enter a username like "<h1>a</h1>" and I see the HTML tag works. So I sent a valid username and Blind XSS Payload on the password field. And In 2–3 hours I got notifications on XSSHunter. 78.180.5.144 and 178.246.104.93 IP addresses are watching the Admin Panel. As you can see the URL is a random string that's why [ffuf](https://github.com/ffuf/ffuf) does not find that URL. So I click that link and there is no authentication mechanism and I can see all accounts attempting to log in on the phishing page.

If 2FA is enabled ( you must enable it ), the script is disabled the 2FA. Because the victim entered the email and password information on the phishing page.

![](https://miro.medium.com/v2/resize:fit:794/0*bGu8IOfaxouh33b9)

Is 2FA open? If it is true, close it

These scammers steal 3 accounts in 3 days. The only thing I can do is delete this information on the website and make people aware.

## Investigating Attackers

You can check the IP Addresses on [whatismyipaddress.com](http://whatismyipaddress.com)

![XSSHunter Report on First Phishing Website](https://miro.medium.com/v2/resize:fit:1400/1*8Y40S0GOWN8lRe-QWkpp3w.png)

> [https://whatismyipaddress.com/ip/178.246.104.93](https://whatismyipaddress.com/ip/178.246.104.93)

![XSSHunter Report on First Phishing Website](https://miro.medium.com/v2/resize:fit:1400/1*hZsMikaiWdPViI9sikFvHg.png)

> [https://whatismyipaddress.com/ip/31.143.156.102](https://whatismyipaddress.com/ip/31.143.156.102)

![XSSHunter Report on First Phishing Website](https://miro.medium.com/v2/resize:fit:1400/1*_1yGGAcvXoxMB_XhvJyEAQ.png)

> [https://whatismyipaddress.com/ip/78.180.5.144](https://whatismyipaddress.com/ip/78.180.5.144)

IP Addresses in Turkey.

![XSSHunter Report on Second Phishing Website](https://miro.medium.com/v2/resize:fit:1116/1*eQMDsPw06wnuYHQZz0hegQ.png)

> [https://whatismyipaddress.com/ip/212.47.230.124](https://whatismyipaddress.com/ip/212.47.230.124)

This IP Address in South Africa

## Phishing Website Admin Dashboard

The Admin Dashboard contains the following information.

-   Username
-   Password
-   2FA Setup Key
-   6 Digit Login Code
-   Changed E-mail Address
-   Backup Codes

![Phishing Website Admin Dashboard](https://miro.medium.com/v2/resize:fit:1384/1*9wjbicYanp-G6E2i3bkAQg.jpeg)

When I click "GoogleAuthenticator & DuoMobile", I was redirected to [https://pilot.albay-arnold.com/Code.php?totp=](https://pilot.albay-arnold.com/Code.php?totp=)<SETUP KEY>. This website generates OTP every 30 seconds and shows it on Admin Dashboard. The website is still up.

After that, I wonder what temporary mail address they used, for understand that they add the "Mail Hesabına Giriş Yap (Login to Mail Account)" button and it redirects to [https://mbox.reispeke6r.com/](https://mbox.reispeke6r.com/).

You can create temporary email addresses on that website. Normally you need a password to see incoming mails but I don't need the password because I can see all of them on Admin Dashboard.

**Example URL :**

`https://mbox.reispeke6r.com/mail/?email=<random string>@mbox.reispeke6r.com&password=<hash>`

On the email parameter, you need to enter your mail address and on the password parameter, there is a hash. When you click that URL, you can see, is the victim's email address replaced by the attacker's email address ? is 2FA disabled? If both of them are true, you can log in victim account.

## Statistics

How many accounts are hacked every hour?

-   X-Plane — Hour
-   Y-Plane — # of Accounts
-   Every dot symbolizes each hour

![Statistics showing accounts hacked per hour](https://miro.medium.com/v2/resize:fit:1400/1*1mE_Ei0E79DDW484tkRK2Q.png)

**Average Age:** 29.53

**Percentage of Gender:** **%60 M — %40 F** ( Except those who don't have a profile picture or fake picture and closed accounts)

## How do I get these informations?

Osintgram offers an interactive shell to perform analysis on the Instagram accounts of any users by its nickname. You can get:

-   addrs Get all registered addressed by target photos
-   captions Get user's photos captions
-   comments Get total comments of target's posts
-   followers Get target followers
-   followings Get users followed by target
-   fwersemail Get email of target followers
-   fwingsemail Get email of users followed by target
-   fwersnumber Get phone number of target followers
-   fwingsnumber Get phone number of users followed by target
-   hashtags Get hashtags used by target
-   info Get target info
-   likes Get total likes of target's posts
-   mediatype Get user's posts type (photo or video)
-   photodes Get description of target's photos
-   photos Download user's photos in output folder
-   propic Download user's profile picture
-   stories Download user's stories
-   tagged Get list of users tagged by target
-   wcommented Get a list of user who commented target's photos
-   wtagged Get a list of user who tagged target`

> [https://github.com/Datalux/Osintgram](https://github.com/Datalux/Osintgram)

I use the "info" command to get the Number of Followers and HD Profile Picture URL. After I receive that information, I write a simple bash script that calculates how many accounts were affected and download the profile pictures.

![Usernames and Dates they were hacked](https://miro.medium.com/v2/resize:fit:1034/1*Q5qwDscE10ujXk2rGVvUaA.jpeg)

![Number of users affected](https://miro.medium.com/v2/resize:fit:624/1*8_K0AVmGJUx3BSSXj63OVA.jpeg)

These statistics are valid for **one day** only. I couldn't follow the other days because their website was down.

After I get the profile pictures, I use the "age-gender-estimation" tool for predicting age and gender information.

![Age and gender estimation results](https://miro.medium.com/v2/resize:fit:864/1*XBf-oVr_L2Ewb5_Aezfi8A.png)

> [https://github.com/yu4u/age-gender-estimation](https://github.com/yu4u/age-gender-estimation)

As a result, do not click on links from unfamiliar sources and do not enter your information. Use two-factor authentication on all possible accounts. Finally, for the sake of awareness, I suggest that they share this article with their friends who use social networks/media.