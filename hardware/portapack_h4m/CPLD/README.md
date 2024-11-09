# Project Setup Guide

## Prerequisites

- Intel Quartus Prime
- Supra-2023.02.b0-7773ca8a-win64-all

## Installation Steps

1. **Project Setup**

- Create a new AG256SL100 project in Quartus Prime
- (Alternatively, copy an existing project)

2. **Code Implementation**

- Implement your code in the project
- Compile the project to verify there are no errors
- Ensure successful compilation before proceeding

3. **Supra Configuration**

- Launch Supra.exe
- Navigate to: File → Project → Open Project
- Select the H4M project when prompted
- Select Tools → Migrate → Next

4. **Additional Setup**

- Follow the on-screen prompts
- You will need to:
    - Open a second Quartus project
    - Execute a provided script

5. **Programming**

- In Supra, go to: Tools → Program
- Click "Query device ID"
- Verify the returned ID is: 0x00025610

6. **File Selection**

- Select the programming file (if not automatically loaded)
- Use the .prg file from the Supra src folder (non-SRAM version)

7. **Programming Process**

- Click "Program" to begin
- Note: The counter shows elapsed seconds, not progress percentage
- Programming is complete when "USB driver disconnected" message appears

## Troubleshooting

If you encounter any issues, ensure:

- All prerequisites are properly installed
- Project configurations are correct
- Device connections are secure