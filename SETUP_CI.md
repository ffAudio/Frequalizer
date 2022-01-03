Setup Github actions
====================

For my own memory this is how the github actions can be configured
to codesign and create an installer. This is mostly learned from this blog post:
https://localazy.com/blog/how-to-automatically-sign-macos-apps-using-github-actions

The code can be seen in this repositories file .github/workflows/workflow.yml

Create certificate
------------------

Create a "Developer ID Application" certificate and import it in your "Keychain Access"
Open Keychain Access and select the certificate and the private key (expand the 
certificate, select both and select "Export 2 items"). Save it as .p12.

Save the p12 as base64 and copy the result in a repository secret MACOS_CERTIFICATE.
Place the password you chose in a repository secret MACOS_CERTIFICATE_PWD.



