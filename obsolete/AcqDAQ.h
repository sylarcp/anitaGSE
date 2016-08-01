// AcqDAQv1.h : includes for acqiris data acquistion for anita-lite
// P. Gorham, started 5/8/2003

// Structure for Acqiris event header
// The original headers are ascii, we convert this over to a
// structure to save space. No bit fields for the moment...



// the following header structure requires currently 96 bytes, but pads to 100
//
//  changed 7/22/03 to accommodate Priority & power spectra info
//  this is versiopn 3 copied over to v2 file name.

typedef struct {
	int evnumber;      // Global event number   4 bytes
	int UTCsec;        // unix UTC   4 bytes
	short  UTCms;      // unix UTC ms  2 bytes
	unsigned short sampint;// sample interval in units of 10 ps  2 bytes
	int  trgdel;       // signed delay in number of samples     4 bytes
	int  Nsamp;        // total number of samples per segment    4 bytes
	char Nseg;         // number of segments in this event--should always =1  1 byte
	char holdoff;      // number of 100ms intervals tohold off between triggers  1 byte
	char Dtype;        // +:triggered; -:timeout, abs(Dtype)= #10sec segs before trigger  1 byte
	char trgcouple;    // flag for type of coupling, 0=DC, 1=AC  1 byte
	char trgslope;     // flag for trigger slope, pos.=0, neg.=1  1 byte
	signed char trgsource; // flag for trigger source, -1=ext 1, -2=ext 2, N=chan N  1 byte
	short trglevel;    // trigger level in 0.5 mV increments   2 bytes
	char clocktype;    // start/stop external=4, external=2, continuous ext.=1, internal=0  1 byte
	short ch_fs[4];      // mV full scale for channels 0-3, using C-type indexing to match code  8 bytes
	short ch_offs[4];    // mV offset for ch 0-3       8 bytes
	char  ch_couple[4];  // flag for type of coupling, ch0-3     4 bytes
	char  ch_bw[4];      // flag for type of bw limit, ch0-3     4 bytes
	signed char Temp0; // temperature in Celsius of module 0      1 byte
	signed char Temp1; // temperature in C if module 1 is separate  1 byte
	signed char ch_mean[4];      //  mean, mV, if abs(mean)>127 we are saturated    4 bytes
	unsigned char ch_sdev[4];    //  standard deviation, mV, saturated at 255  4 bytes
        char GPSday ;  // day of week [1-7], -1 means no GPS data for this ev.  1 byte
        char GPShour ; // hour from GPS       1 byte
        char GPSmin ;  // minnute from GPS    1 byte
        char GPSsec ;  // second              1 byte
        int  GPSfsec ; // fraction of second in 100ns unit.  4 bytes
        float GPSlat ;  // GPS latitude  (ddmm.mmm, N ; positive)   4 bytes
        float GPSlong ; // GPS longitude  (dddmm.mm, E ; positive)   4 bytes
        float GPSalt ;  // GPS altitude  (in m, -9999. means no position data)  4 bytes
	signed char meanpwr[4]; // mean in band power in spectrum, dB V^2/MHz   4 bytes
	signed char peakpwr[4]; // highest power in dB V^2/MHz                  4 bytes
	unsigned char peakf[4];   //frequency of peak power, 4 MHz units, 0-255   4 bytes
	char Priority;  // event priority: 0: housekeep or forcetrig, 1-4 coinc level; 5 timeout     1 byte
	                // 6 4/3-fold, veto present, 7 2/1-fold veto present, 8=0-fold, 9= argos
	}
	ACQHEADER;
