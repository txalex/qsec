using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.IO;


namespace WindowsFormsApplication
{
    public partial class Form1 : Form
    {
        // callback event constants
        const int CLLBK_CARD_REMOVED = 2;
        const int CLLBK_CARD_INSERTED = 3;
        const int CLLBK_READ_SUCCESS = 4;
        const int CLLBK_CARD_INVALID = 5;
        const int CLLBK_ERR = 6;
        const int CLLBK_READER_CHANGE = 7;

        // mifare classic constants
        private const byte KEY_A = 0x60;
        private const byte KEY_B = 0x61;
        private readonly byte[] DEFAULT_KEY = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
        private readonly byte[] DEFAULT_ACCESS_BITS = { 0xFF, 0x07, 0x80, 0x69 };
        private const int LEN_BYTE_BLOCK = 16;
        private const int LEN_BYTE_ACCESS_BITS = 4;
        private const int OFFSET_BYTE_ACCESS_BITS = 6;
        private const int LEN_KEY = 6;
        private const int OFFSET_BYTE_KEY_B = 10;

        private const int NUM_KEYS = 2;

        // # of blocks in a given sector type
        private const int NUM_DATA_BLOCKS_MANUFACTUR_SECTOR = 2;
        private const int NUM_DATA_BLOCKS_BIG_SECTOR = 15;
        private const int NUM_DATA_BLOCKS_LITTLE_SECTOR = 3;

        // # of sectors of a given type
        private const int NUM_MANU_SECTORS = 1;
        private const int NUM_4K_LITTLE_SECTORS = 31;
        private const int NUM_4K_BIG_SECTORS = 8;
        private const int NUM_4K_SECTORS = 40;
        private const int NUM_4K_BLOCKS = 256;

        // total available # of data blocks
        private const int LEN_4K_USABLE_BLOCKS = NUM_4K_BLOCKS - NUM_4K_SECTORS;

        // total available # of data bytes
        private const int LEN_4K_USABLE_BYTES = LEN_4K_USABLE_BLOCKS * 16;

        // card type constants
        private const int MF_1K = 0;
        private const int MF_4K = 1;

        //  tx error returns
        private const int ERR_PCSC_FAILED = 0x104;
        private const int ERR_INVALID_OFFSET = 0x105;

        // used to track which reader was presented a card
        string szReader;

        public delegate void callbackDelegate(int iEvent, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 2)] byte[] abReaders, uint uiLen, int iCrdType);
        private callbackDelegate callbackDelegateInstance;

        // start a polling worker thread
        [DllImport("txcrdwrppr.dll", EntryPoint = "ExtStartPolling")]
        private static extern bool ExtStartPolling(callbackDelegate managedCallbackHandler);

        // end the polling worker thread
        [DllImport("txcrdwrppr.dll", EntryPoint = "ExtEndPolling")]
        private static extern bool ExtEndPolling();

        // enables/disables debug output logging
        [DllImport("txcrdwrppr.dll", EntryPoint = "ExtOutputLogging")]
        private static extern uint ExtOutputLogging(bool bLogging);

        // set a specific log path for debug output.  it's not required to call this
        // default is c:\txlog.txt
        [DllImport("txcrdwrppr.dll", EntryPoint = "ExtSetLogPath", CharSet = CharSet.Unicode)]
        private static extern uint ExtSetLogPath(string szPath);

        [DllImport("txcrdwrppr.dll", EntryPoint = "ExtGetCSN")]
        private static extern uint ExtGetCSN(
                                        [MarshalAs(UnmanagedType.LPWStr)] string szReaderName,  // IN - reader where the card was presented
                                        byte[] baCSN,   // OUT - CSN array
                                        ref int iLen);  // OUT - length of CSN 

        // updates keys A and B and the access bits
        [DllImport("txcrdwrppr.dll", EntryPoint = "ExtUpdateSectorTrailers")]
        private static extern uint ExtUpdateSectorTrailers(
                                            [MarshalAs(UnmanagedType.LPWStr)] string szReaderName,  // IN - reader where the card was presented
                                            byte[] baCurrentKey,    // IN - current Key
                                            byte bAuthKey,          // IN - Key A or Key B
                                            [In][MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.I1)] byte[, ,] pArrayOfByte, // IN - key array
                                            byte[] baAccessBits);   // IN - new access bits


        // reads the card
        [DllImport("txcrdwrppr.dll", EntryPoint = "ExtMfReadCard")]
        private static extern uint ExtMfReadCard(
                                        [MarshalAs(UnmanagedType.LPWStr)] string szReaderName,  // IN - reader where the card was presented
                                        byte[] baKey,       // IN - security key 
                                        byte bKeyType,      // IN - key type, A or B
                                        byte[] baData,      // OUT - data to read
                                        int iBlockOffest,   // IN - block offset 
                                        int iLen);          // IN - length to read - the offsset is blocks, not bytes, length is bytes and will be padded to a multiple of 16

        // writes the card
        [DllImport("txcrdwrppr.dll", EntryPoint = "ExtMfWriteCard")]
        private static extern uint ExtMfWriteCard(
                                        [MarshalAs(UnmanagedType.LPWStr)] string szReaderName,  // IN - reader where the card was presented
                                        byte[] baKey,       // IN - security key
                                        byte bKeyType,      // IN - key type, A or B
                                        byte[] baData,      // IN - data to write
                                        int iBlockOffest,   // IN - block offset
                                        int iLen);          // IN - length to write - the offsset is blocks, not bytes, length is bytes and will be padded to a multiple of 16
        public Form1()
        {
            InitializeComponent();
            this.Load += new EventHandler(this.Form1_Load);
            ExtSetLogPath("C:\\test\\txmflog.txt");
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            bool bLog = true;

            // initiates debugging
            ExtOutputLogging(bLog);

            if (bLog)
                ExtSetLogPath("C:\\test\\txlog.txt");

            // callback function
            callbackDelegateInstance = new callbackDelegate(CallbackHandler);

            // sets a value indicating whether to catch calls on the wrong thread 
            // that access a control's Handle property when an application is being debugged. 
            Button.CheckForIllegalCrossThreadCalls = false;

            // starts a worker thread that handles card and reader polling, passing in callback
            if (ExtStartPolling(callbackDelegateInstance))
            {
                btnStrtPllng.Enabled = false;
                btnNdPllng.Enabled = true;
            }
        }

        private void EnableCardButtons()
        {
            btnRd.Enabled = true;
            btnWrt.Enabled = true;
            btnUpdtSctrTrlrs.Enabled = true;
            btnGtCSN.Enabled = true;
        }

        private void DisableCardButtons()
        {
            btnRd.Enabled = false;
            btnWrt.Enabled = false;
            btnUpdtSctrTrlrs.Enabled = false;
            btnGtCSN.Enabled = false;
        }

        // receives card/reader polling events
        private void CallbackHandler(int iEvent, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 2)] byte[] abReaders, uint uiReaderLen, int iCrdType)
        {
            switch (iEvent)
            {
                case CLLBK_READER_CHANGE:	// when a reader insertion/removal has occured
                    if (uiReaderLen == 0)
                    {
                        szReader = "";
                        DisableCardButtons();
                    }
                    break;

                case CLLBK_CARD_INSERTED:	// when a card is inserted
                    szReader = Encoding.ASCII.GetString(abReaders, 0, (int)uiReaderLen);
                    EnableCardButtons();
                    break;

                case CLLBK_CARD_REMOVED:	// when a card is removed
                    DisableCardButtons();
                    break;

                case CLLBK_CARD_INVALID:	// when an invalid card is inserted -- nothing implemented here yet
                    DisableCardButtons();
                    break;

                default:
                    break;
            }
        }

        private void btnNdPllng_Click(object sender, EventArgs e)
        {
            // set an event to end the polling
            if (ExtEndPolling())
            {
                btnStrtPllng.Enabled = true;
                btnNdPllng.Enabled = false;
                DisableCardButtons();
            }
        }

        private void btnStrtPllng_Click(object sender, EventArgs e)
        {
            // starts a worker thread that handles card and reader polling, passing in callback
            if (ExtStartPolling(callbackDelegateInstance))
            {
                btnStrtPllng.Enabled = false;
                btnNdPllng.Enabled = true;
            }
        }

        private void btnUpdtSctrTrlrs_Click(object sender, EventArgs e)
        {
            // sector trailer access bits
            // byte 6 = 0 1 1 1 1 0 0 0 
            // byte 7 = 0 1 1 1 0 1 1 1 
            // byte 8 = 1 0 0 0 1 0 0 0
            byte[] baAccessBits = new byte[] { 0x78, 0x77, 0x88, 0x69 };    // production access bits
            uint uiRet = 0;

            // define 40 sector key set
            byte[, ,] baArrayKeys = new byte[NUM_4K_SECTORS, NUM_KEYS, LEN_KEY]{
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           //{ {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}, {0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} }, 
                           { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} } };

            // since the 3D array is marshaled as a C-style array, a pointer to the internal array buffer of the managed array is passed to the unmanaged API
            GCHandle gch = GCHandle.Alloc(baArrayKeys, GCHandleType.Pinned);
            IntPtr pArrayData = gch.AddrOfPinnedObject();
            uiRet = ExtUpdateSectorTrailers(szReader, DEFAULT_KEY, KEY_A, baArrayKeys, DEFAULT_ACCESS_BITS);
            gch.Free();
        }

        private void btnRd_Click(object sender, EventArgs e)
        {
            uint uiRet = 0;
            int iBlockOffset = 4;           // arbitrary offset
            byte[] baRead = new byte[33];   // arbitrary length

            if (0 != (uiRet = ExtMfReadCard(szReader, DEFAULT_KEY, KEY_A, baRead, iBlockOffset, baRead.Length)))
            {
                // some error, check the debug output file for details
                return;
            }
        }

        private void btnWrt_Click(object sender, EventArgs e)
        {
            uint uiRet = 0;
            int iBlockOffset = 0; // arbitrary offset

            // data string examples
            string szWrite = "00000000000000000000000000000000";
            //string szWrite = "1234567890ABCDEF1234567890ABCDEF";
            //string szWrite = "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" +
            //                 "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" +
            //                 "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" +
            //                 "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" +
            //                 "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" +
            //                 "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" +
            //                 "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
            //string szWrite = "abcdefghijklmnopqrstuvwxyz01234567890abcdefghijklmnopqrstuvwxyz012345abcdefghijklmnopqrstuvwxyz012345abcdefgh" +
            //                 "abcdefghijklmnopqrstuvwxyz01234567890abcdefghijklmnopqrstuvwxyz012345abcdefghijklmnopqrstuvwxyz012345abcdefgh" +
            //                 "abcdefghijklmnopqrstuvwxyz01234567890abcdefghijklmnopqrstuvwxyz012345abcdefghijklmnopqrstuvwxyz012345abcdefgh" +
            //                 "abcdefghijklmnopqrstuvwxyz01234567890abcdefghijklmnopqrstuvwxyz012345abcdefghijklmnopqrstuvwxyz012345abcdefgh" +
            //                 "abcdefghijklmnopqrstuvwxyz01234567890abcdefghijklmnopqrstuvwxyz012345abcdefghijklmnopqrstuvwxyz012345abcdefgh" +
            //                 "abcdefghijklmnopqrstuvwxyz01234567890abcdefghijklmnopqrstuvwxyz012345abcdefghijklmnopqrstuvwxyz012345abcdefgh" +
            //                 "abcdefghijklmnopqrstuvwxyz01234567890abcdefghijklmnopqrstuvwxyz012345abcdefghijklmnopqrstuvwxyz012";

            byte[] baWrite = new byte[szWrite.Length];
            baWrite = Encoding.ASCII.GetBytes(szWrite);

            if (0 != (uiRet = ExtMfWriteCard(szReader, DEFAULT_KEY, KEY_A, baWrite, iBlockOffset, baWrite.Length)))
            {
                // some error, check the debug output file for details
                return;
            }
        }

        private void btnGtCSN_Click(object sender, EventArgs e)
        {
            uint uiRet = 0;
            byte[] baRead = new byte[10];
            int iLen = 0;

            if (0 != (uiRet = ExtGetCSN(szReader, baRead, ref iLen)))
            {
                // some error, check the debug output file for details
                return;
            }
        }
    }
}
