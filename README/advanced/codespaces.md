## Codespaces

1. Click the "**<> Code**" button.
2. Select "**Codespaces**".
3. Choose "**Create codespace on main**" to create a new Codespace on the *main* branch.
4. Once the Codespace opens in the VSCode interface:
- Click the **three-line menu** (≡) in the top-left corner.
- Select **Terminal**.
- Click **New Terminal**.
- In the terminal, you can drag & paste:
```yaml
sudo apt update && sudo apt upgrade &&
sudo apt install make &&
sudo make && make linux &&
chmod +x watchdogs &&
./watchdogs
```