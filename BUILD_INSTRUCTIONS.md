# How to Build and Run GameOfLife

## Method 1: Using Terminal (Easiest & Most Reliable)

### Step 1: Open Terminal in VS Code
- Press `` Ctrl+` `` (backtick key, usually above Tab) to open the terminal
- OR go to: **Terminal** → **New Terminal** from the menu

### Step 2: Build the Project
Type this command and press Enter:
```
build-debug.bat
```

### Step 3: Run the Game
After build succeeds, type:
```
x64\Debug\GameOfLife.exe
```

---

## Method 2: Using VS Code Command Palette

### Step 1: Open Command Palette
- Press `Ctrl+Shift+P` (or `F1`)

### Step 2: Run Build Task
- Type: `Tasks: Run Task`
- Press Enter
- Select: `build-debug` or `build-release`
- Press Enter

### Step 3: Run the Executable
- Press `F5` to debug, OR
- Open terminal and type: `x64\Debug\GameOfLife.exe`

---

## Method 3: Using Run Menu

### Step 1: Build
- Go to: **Terminal** → **Run Task...**
- Select: `build-debug`

### Step 2: Run
- Press `F5` (this will build first, then run)
- OR go to: **Run** → **Start Debugging**

---

## Method 4: Double-Click Batch File (Windows Explorer)

1. Open Windows File Explorer
2. Navigate to your project folder: `C:\Users\cason\OneDrive\Desktop\CODING\C++\SFML\Projects\GameOfLife`
3. Double-click `build-debug.bat`
4. Wait for "Build successful!" message
5. Navigate to `x64\Debug\` folder
6. Double-click `GameOfLife.exe` to run

---

## Quick Reference

**Build Debug:**
```
build-debug.bat
```

**Build Release:**
```
build-release.bat
```

**Run Debug:**
```
x64\Debug\GameOfLife.exe
```

**Run Release:**
```
x64\Release\GameOfLife.exe
```

---

## Troubleshooting

**If build fails with "Visual Studio not found":**
- Make sure Visual Studio 2019 or 2022 is installed with C++ support
- The script automatically finds Visual Studio, but if it fails, you may need to install it

**If you get "cl.exe is not recognized":**
- The batch file should handle this automatically
- If it doesn't work, try opening "Developer Command Prompt for VS" and running the batch file from there

**If DLL errors when running:**
- Make sure you're running from the `x64\Debug` or `x64\Release` folder
- The DLLs should already be there, but if not, copy them from the root `x64\Debug` or `x64\Release` folders

