/* ************************************************************************** */
/* * bcnrand.inl part of bcnrand.h                                                              * */
/* * Copyright (C) 2012 Deakin University                                   * */
/* * Authors: Gleb Beliakov, Tim Wilkin, Michael Johnstone                  * */
/* * Created: 08/06/12     Last Modified: 27/04/14                          * */
/* ************************************************************************** */


/*
	Constants used in Barrett reduction algorithm
*/
static const uint64_t	BCN_a    = ULL(33554432);				/* a = 2^25  */
static const uint64_t	BCN_m    = ULL(5559060566555523);		/* m = 3^33  */
static const uint64_t	BCN_r    = ULL(5946243);				/* r = m % a */
static const uint64_t	BCN_mulo = ULL(0x33D9481681D79D);
static const uint64_t	BCN_q    = ULL(165672915);              /* m div a   */
static const uint64_t	BCN_t	 = ULL(2779530283277761);		/* floor(m/2) */

static const double	BCN_qinv = 6.0359896486399119614693807977001e-9;	/* 1.0 / (double)(BCN_q); */
static const double	BCN_minv = 1.7988650924514300510763861722128e-16;	/* 1.0 / (double)(BCN_m); */
static const double BCN_b	 = 9007199254740992.0;						/* 2^53 */

/*
	Constants used in lcg
*/
static const int64_t 	LCG_m 		= 2147483649;								/* m = 2^31 + 1  	*/
static const int64_t 	LCG_m1 		= 2147483648;								/* m - 1			*/
static const int64_t 	LCG_a 		= 39373;									/* a = 39373  		*/
static const int64_t 	LCG_q 		= 54542;									/* q = floor m / a  */
static const double 	LCG_qinv 	= 1.8334494517986139122144402478824e-5; 	/* 1/q 				*/
static const int64_t 	LCG_r 		= 1483;										/* r =  m mod a  	*/
static const double 	LCG_m_inv 	= 4.6566128709089882341637330901978e-10;	/* 1/m 				*/
static const int64_t	LCG_t		= 1073741824;								/* floor(m/2) */


#define BCN_t53 9007199254740992						//exp2(53.0);
//#define BCN_tp 1.6202736320108106429874177483778
//#define BCN_t  2779530283277761;		/* floor(m/2) */


//Macros for each of the methods

#define lcn8(s)\
 	T1 = (s)*BCN_qinv;\
 	s =  (((s)-T1*BCN_q)<<25 )- T1*BCN_r;\
	s+=BCN_m;

#define lcn(s)\
 	T1 = (s)*BCN_qinv;\
 	s =  (((s)-T1*BCN_q)<<25 )- T1*BCN_r;\
	if (s < 0)	s+=BCN_m;

#define barrett_step_simple(rlo)\
	xlo = BCN_t53 * rlo;\
	xhi = __umul64hi (BCN_t53, rlo);\
	qlo = (xhi << 12) | (xlo >> 52);\
	qhi = __umul64hi(qlo, BCN_mulo);\
	qlo = qlo * BCN_mulo;\
	qlo = (qhi << 10) | (qlo >> 54);\
	qhi = 0;\
	r1lo = xlo & 0x3FFFFFFFFFFFFFULL;	\
	r2lo = qlo * BCN_m;\
	r2lo = r2lo & 0x3FFFFFFFFFFFFFULL;\
	rlo = r1lo - r2lo;\
	if (r1lo<r2lo) rlo += 0x40000000000000ULL;\
	while (rlo >= BCN_m) rlo -= BCN_m;

#define barrett_step_opt(rlo)\
	qhi = __umul64hi(rlo, BCN_mulo);\
	qlo = rlo * BCN_mulo;\
	r2lo = (((qhi << 11) | (qlo >> 53)) * BCN_m)& 0x1FFFFFFFFFFFFFULL;\
	rlo = 0x20000000000000ULL - r2lo;\
	while (rlo >= BCN_m) rlo -= BCN_m;


__device__ __inline__	uint64_t	umul64(uint64_t	a, uint64_t	b, uint64_t	*hi)
{
	*hi = __umul64hi(a,b);
	return (a * b);			
}

/*!
 ---------------------------------------------
	Function: BarrettStep

	INPUTS
		r:	64 bit unsigned integer type containing
			the current iterate of the seed algorithm
			of the generator
	q_t53:	64 bit unsigned integer type containing
			the constant 2^2^i % 3^33 at the i'th
			step in seeding the generator
	OUTPUTS
			64 bit unsigned integer type containing
			the next iterate of the generator (z_k+1)
 ---------------------------------------------
*/

__device__ __inline__ uint64_t	BarrettStep(
	uint64_t	z,
	uint64_t	q_t53
	)
{
	uint64_t	r1lo, r2lo;

#if defined(NATIVE_128BIT_INT_TYPES)
	uint128_t	x, q;
	x = umul128(z,q_t53);
	q = umul128((x>>52), BCN_mulo);
	r2lo = (q >> 54) * BCN_m;
 	r1lo = x & ULL(0x3FFFFFFFFFFFFF);

#else
	uint64_t	xlo, xhi,
				qlo, qhi;
	
	xlo = umul64(z, q_t53, &xhi);

	qlo = (xhi << 12) | (xlo >> 52);
	qlo = umul64(qlo, BCN_mulo, &qhi);
	qlo = (qhi << 10) | (qlo >> 54);

	r1lo = xlo & ULL(0x3FFFFFFFFFFFFF);
	r2lo = qlo * BCN_m;

#endif

	r2lo &= ULL(0x3FFFFFFFFFFFFF);
	z = r1lo - r2lo;
	if (r1lo < r2lo)
		z += ULL(0x40000000000000);

	while (z >= BCN_m)
		z -= BCN_m;

	return z;
}










/*	
 * LCN_Inline
 * The inline LCN kernel - generates the next random number in the sequence based on the provided seed
 */
__device__ inline int64_t LCN_Inline(int64_t seed)
{
	int64_tt T1;
	
	seed<<=2;	
 	lcn8(seed);
	seed<<=1;	
	lcn(seed);	
	return seed;
}










/*!
 ---------------------------------------------
	Function: BarrettInitBit

	INPUTS
		k:	64 bit unsigned integer containing the
			lcg generator seed value in the range
			[0,2^53 - 3^33 - 100]
	OUTPUTS
			64 bit unsigned integer type containing
			the final iterator of the lcg seed
			algorithm
 --------------------------------------------- 
	NOTES
			Uses alternative seeding algorithm based
			on precomputed iterates for 5 bit subsets
			of the seed, k
 --------------------------------------------- 
*/
__constant__ uint64_t BCN_R[11][32] = 
	{
		{ 1ULL,       4294967296ULL, 1781113878326302ULL, 5026729391779105ULL, 2686603886329009ULL, 2824462036602025ULL, 2789598520605340ULL, 1982981359282135ULL, 5528797009494424ULL, 4611673145862217ULL, 4000916771807359ULL, 4530895128526264ULL, 2208900989478376ULL,  649590795214915ULL, 4900856764565131ULL, 3328448703661441ULL,  997699201948327ULL, 1494181581904366ULL, 3587074781336110ULL, 5075187893368492ULL, 1294987753320235ULL, 1233096543616897ULL, 2950405819077775ULL, 2315373552273679ULL, 4178346224247127ULL, 5389913925505975ULL, 1149022478377744ULL, 3334798063541719ULL, 5558337275595304ULL, 3206169515365036ULL, 1033839917266438ULL,  744355112214388ULL },
		{ 1ULL, 3732059326150627ULL, 5215019856241441ULL, 3705342047580556ULL, 1919989699093888ULL, 4010013445630774ULL, 4583661214445236ULL, 2667304620466252ULL, 1673704953907087ULL,  179555604053446ULL, 4329238730438320ULL, 3296044013194309ULL, 1233166988231965ULL,  488910328081756ULL, 4641558067355089ULL,  567575385801706ULL, 4612967458491778ULL,  825221669867122ULL, 2950162089715972ULL,  359411994922669ULL,  248625533653177ULL, 1017842955898627ULL, 2233762391404339ULL, 4470276271995292ULL, 2909771746715890ULL, 3360234883958413ULL, 1430143800593062ULL, 2691712178951842ULL, 4235027186984002ULL, 3759499019009677ULL, 3842652948916303ULL, 2656575796316434ULL},
		{ 1ULL, 2283734602837201ULL, 4479056590352335ULL, 2843538581847196ULL, 4252816925065471ULL, 4522069863263023ULL, 2448137822643559ULL, 2186292947359756ULL, 2040699666820606ULL, 2551259785631014ULL, 5824847576849800ULL, 3648142363569793ULL, 3724542619031683ULL, 4042064167484152ULL, 2860915431885973ULL, 4881506778113626ULL, 3784178533866853ULL, 1914690258769255ULL, 4405782376600777ULL, 1398014377592440ULL, 1848420369554245ULL, 1942638229935235ULL, 4144350571449583ULL, 1851498183395167ULL, 5275710731110267ULL, 2433287605236844ULL, 4114908317537683ULL, 1123319824254775ULL, 2116098473938789ULL, 1268205454354804ULL, 5458375025539120ULL, 2567579298179935ULL },
		{ 1ULL, 1213687325949946ULL, 3002103258434401ULL,  359909622261784ULL, 4358657226425659ULL, 3884340187080724ULL, 1673429008954510ULL, 5481415260390472ULL,  379021759686229ULL, 2499819374558314ULL, 2392121708963152ULL, 3155160414366859ULL, 3781933249587139ULL, 2447789189902162ULL, 2978019705903691ULL, 4326166884148480ULL, 2489111096968006ULL,  760688680022626ULL, 1853297574109543ULL, 3324617165204350ULL, 2822341452832426ULL, 4198073747284117ULL, 4445080672922476ULL, 5137695930535726ULL, 5120185238881660ULL, 1138245969970942ULL, 3670336691929066ULL, 2727987109029091ULL, 4596042367378978ULL, 1860262624906897ULL, 3643025487797224ULL, 5109409545659251ULL },
		{ 1ULL,  953790170100016ULL,   72149290184617ULL, 1436930057556649ULL, 1639508129396500ULL,  293407407147391ULL,   83707215346288ULL, 1314189672355975ULL, 3932865070361773ULL, 1166370679230661ULL, 1710475147025428ULL, 2227214295539089ULL, 2091414038644225ULL, 3424886667549859ULL, 1380459105965005ULL, 2222478772463053ULL, 2101443068986582ULL, 1254739221210634ULL,  474526843804243ULL, 3214425666298045ULL, 3861200559346261ULL, 2710077009391117ULL, 1080127965855361ULL, 1918550190429961ULL, 2933356960148482ULL, 2814972882050920ULL, 2728539809213470ULL, 2538955180080406ULL, 1878693244578580ULL, 4955431683098815ULL, 5434680611403793ULL, 2757455210165503ULL },
		{ 1ULL, 5185180911217255ULL, 5513947978920745ULL,  186062409679276ULL, 4412631187856788ULL,  292118194339564ULL, 3658816613516374ULL, 4069432445482654ULL, 3206708381470054ULL, 4805096162361949ULL, 1534052361665734ULL, 2117281814496400ULL, 5542896851092414ULL, 2190502158444907ULL, 2542428363235423ULL, 4885324482729529ULL, 3821899512320746ULL, 4320442494449167ULL, 2651047696014157ULL, 4170359291655058ULL,  637627643977630ULL, 3648015536609530ULL, 1835460776865967ULL, 1206539575571587ULL, 4375501860620998ULL, 4906810424302396ULL,  539768452719883ULL, 3299766327589123ULL, 2681630425154806ULL, 4609003201767553ULL,  715367594459017ULL, 3612506555932216ULL },
		{ 1ULL, 4400940732530035ULL, 2251259616847777ULL, 2865682930252978ULL,  387003850566844ULL,  428107748732476ULL, 4905982863735580ULL,  912452720592766ULL, 4384891116727054ULL, 5209535883346642ULL, 2844318229596247ULL, 1975375356548065ULL, 4067598052337626ULL, 2304628425257062ULL,  933325136424727ULL, 2394515698788241ULL,  657138699980563ULL, 3111401057248060ULL, 4701301577221141ULL,  736268572718860ULL, 5169398800123339ULL, 1890211522821967ULL, 5546305445916568ULL, 3117757527294766ULL,   43491719599516ULL, 2796618785408509ULL, 4090212028817728ULL, 2463515510152195ULL, 3621354182270509ULL, 1577647703681818ULL, 3641059824848929ULL, 3189901901146915ULL },
		{ 1ULL, 5372432711697328ULL,  431271224235886ULL, 3926060904985732ULL, 4538284711740799ULL, 3170180950838707ULL, 1697498361906922ULL, 4720028504770636ULL, 1800985404135163ULL, 3133984399133536ULL, 1507168510374580ULL, 2864927573135815ULL,  942584358641032ULL, 4678472638573819ULL, 3517073724829108ULL, 3798941613760459ULL, 4706358994471642ULL,   44894198382268ULL, 3785332018740067ULL, 4911804015601561ULL, 3298670402790127ULL, 1754207199272449ULL, 4375628060482852ULL, 2606391516921034ULL,  326014088690296ULL, 1996170211215475ULL, 1953494969515690ULL, 1853392700602828ULL, 3451452869184559ULL, 2222667662462977ULL, 1012949411019166ULL, 4811558423231698ULL },
		{ 1ULL, 4417152808561177ULL, 3630104538274294ULL,  284118943281571ULL, 1840063802930086ULL, 2122217264408893ULL, 1931374135227727ULL, 3598147549709320ULL, 2765825240192080ULL, 5288806640982520ULL, 3104302402597306ULL,  811363552126720ULL, 3903095184045697ULL, 1742577767620960ULL, 4419588337778671ULL, 1791593052582700ULL, 4666182094710670ULL, 1296981349742086ULL, 4740994627114870ULL, 2044314408216163ULL, 1435993555420897ULL, 2845733269499173ULL, 2183910894354028ULL, 4885369983629575ULL,  259206505486075ULL, 4356636151000720ULL, 2289506392555198ULL, 1966221794591020ULL, 3937814545088731ULL,  678659999139151ULL, 4453092118359037ULL, 2428977584372035ULL },
		{ 1ULL,  480336872876206ULL, 3085515470269723ULL, 4558372979184082ULL, 5146488153698920ULL, 4536979174751341ULL, 1062447008180230ULL, 4591198493444536ULL, 3790769725497532ULL, 4181246534731735ULL, 1353382494667063ULL, 4699338356739145ULL, 2162423356077187ULL,   89375730437158ULL, 3308792498371246ULL, 3747956062651453ULL, 4181643439999999ULL, 3230898332232136ULL,  504142986723886ULL, 4319960319308401ULL,  941275446809566ULL, 4568529119823730ULL, 2429443796780608ULL,  525152338236076ULL, 1832934482258257ULL, 5053328927433709ULL, 4421604550408276ULL,  314093303160103ULL, 5229302609160196ULL, 2359905264339547ULL, 2943745299399169ULL,  947079196737910ULL },
		{ 1ULL, 3635766119061013ULL, 5382313195644475ULL, 1899031547876788ULL, 2608018674565117ULL, 3028221017926936ULL, 2955837911505616ULL, 5552877857653453ULL,  593603128023631ULL, 3050048539707490ULL, 2088171611530600ULL,  924314838049429ULL, 2136591884839906ULL,  185797444448272ULL,  850470568427977ULL,  390466373029408ULL, 1817401114838200ULL, 4213755003737968ULL, 5224608890973136ULL, 1431382635133339ULL, 3514755340776139ULL, 3478597870870999ULL,  463975388090338ULL, 3080379655274686ULL,  890859006727777ULL, 1132766991223582ULL,  168863053170832ULL, 3052662708181690ULL, 4428574830921352ULL, 5117848555557769ULL, 4143048507448885ULL,  582561282442792ULL}
	};

__device__ __inline__ uint64_t BarrettInitBit(uint64_t k)
{
	uint32_t	i = 0;
	uint64_t	q = 0x1ULL << (k & 0x1F);

	for (i = 0; i < 11; i++)
	{
		k = k >> 5;
		q = BarrettStep(q,BCN_R[i][k & 0x1F]);
	}

	return BarrettStep(q,BCN_t);
}



/* ========= auxiliary generator =============*/

/*!
 ---------------------------------------------
	Function: LCGStep

	INPUTS
		s:	64 bit unsigned integer type containing
			the current iterate of the seed algorithm
			of the generator
		a:	64 bit unsigned integer type containing
			the constant 2^2^i % 3^33 at the i'th
			step in seeding the generator
	OUTPUTS
			64 bit unsigned integer type containing
			the next iterate of the generator (z_k+1)
 --------------------------------------------- 
*/
__device__ __inline__ uint64_t	LCGStep(uint64_t	s, uint64_t	a)
{
	return (a * s) % LCG_m;
}

/*!
 ---------------------------------------------
	Function: LCGInitBit

	INPUTS
		k:	64 bit unsigned integer containing the
			lcg generator seed value in the range
			[0,2^31-1]
	OUTPUTS
			64 bit unsigned integer type containing
			the final iterator of the lcg seed
			algorithm
 --------------------------------------------- 
*/

__constant__ uint64_t BCN_AUX_R[12][32] = 
	{
		{ 1ULL,39373ULL,1550233129ULL,1548716239ULL,1953748441ULL,2073060313ULL,1045172557ULL,1497404623ULL,296121733ULL,512262988ULL,164195116ULL,928518778ULL,1955689267ULL,1179791047ULL,1841565661ULL,326845717ULL,1174390633ULL,1811946490ULL,214847341ULL,246263782ULL,255213451ULL,443212552ULL,155678122ULL,596363260ULL,24417814ULL,1477399519ULL,761661124ULL,1421760616ULL,524455285ULL,1322651170ULL,266028160ULL,1048987507ULL},
		{ 1ULL,1379575543ULL,469631365ULL,389425342ULL,198127207ULL,918318175ULL,1450073974ULL,731533042ULL,1810121248ULL,302834266ULL,1442855539ULL,2095933756ULL,983833330ULL,1995893602ULL,1353053620ULL,208372447ULL,1059278440ULL,988318147ULL,638019247ULL,921788956ULL,395313439ULL,1015947829ULL,711758293ULL,2119292047ULL,369656308ULL,1369050772ULL,573829942ULL,1554724ULL,1930017859ULL,566174611ULL,270318442ULL,216646957ULL},
		{ 1ULL,424591189ULL,1003159306ULL,1911076207ULL,2045675353ULL,1227931009ULL,1919393464ULL,155474929ULL,518277262ULL,118911409ULL,80613451ULL,528099832ULL,339131815ULL,563048662ULL,1784772466ULL,2083404574ULL,302749087ULL,1012593034ULL,1787118595ULL,987251272ULL,294603064ULL,2144030140ULL,260292436ULL,1862593549ULL,599969860ULL,1253561788ULL,1677941752ULL,1619354071ULL,1270183609ULL,893021566ULL,1677014089ULL,501706801ULL},
		{ 1ULL,210843040ULL,1228916023ULL,1017483766ULL,1561646362ULL,1606026670ULL,764971363ULL,73422361ULL,499383703ULL,1637457181ULL,492901246ULL,1697699494ULL,755836393ULL,1100846926ULL,2005827727ULL,854587087ULL,2119291921ULL,1095112225ULL,1554713296ULL,718973032ULL,1278847888ULL,2040801448ULL,1580912197ULL,424266418ULL,963374608ULL,1294653277ULL,1769360404ULL,1688794414ULL,1636284979ULL,1605406927ULL,204423346ULL,1636632718ULL},
		{ 1ULL,566549329ULL,1761804235ULL,830296516ULL,781918015ULL,1908415615ULL,1751331013ULL,40163332ULL,1830950671ULL,724860829ULL,2056631389ULL,1317437608ULL,2089220989ULL,1739796649ULL,156977629ULL,365854993ULL,184785544ULL,1720606414ULL,1188023044ULL,181218922ULL,569301730ULL,1072091953ULL,542897182ULL,593260111ULL,1556366704ULL,1120371469ULL,1066562152ULL,1410311230ULL,566145355ULL,780546613ULL,1052185504ULL,195784390ULL},
		{ 1ULL,730155415ULL,927030997ULL,880356631ULL,963694168ULL,1955666011ULL,1781280952ULL,1299434806ULL,1233838864ULL,167395729ULL,658476988ULL,2043554560ULL,1349796943ULL,1119812503ULL,1344654268ULL,1590665242ULL,207904591ULL,859184938ULL,1362614689ULL,1426583842ULL,1261614826ULL,219367078ULL,1196419747ULL,159106633ULL,658526563ULL,1513865641ULL,613900969ULL,1489595701ULL,1414293565ULL,1288859164ULL,535969462ULL,125189686ULL},
		{ 1ULL,39373ULL,1550233129ULL,1548716239ULL,1953748441ULL,2073060313ULL,1045172557ULL,1497404623ULL,296121733ULL,512262988ULL,164195116ULL,928518778ULL,1955689267ULL,1179791047ULL,1841565661ULL,326845717ULL,1174390633ULL,1811946490ULL,214847341ULL,246263782ULL,255213451ULL,443212552ULL,155678122ULL,596363260ULL,24417814ULL,1477399519ULL,761661124ULL,1421760616ULL,524455285ULL,1322651170ULL,266028160ULL,1048987507ULL},
		{ 1ULL,1379575543ULL,469631365ULL,389425342ULL,198127207ULL,918318175ULL,1450073974ULL,731533042ULL,1810121248ULL,302834266ULL,1442855539ULL,2095933756ULL,983833330ULL,1995893602ULL,1353053620ULL,208372447ULL,1059278440ULL,988318147ULL,638019247ULL,921788956ULL,395313439ULL,1015947829ULL,711758293ULL,2119292047ULL,369656308ULL,1369050772ULL,573829942ULL,1554724ULL,1930017859ULL,566174611ULL,270318442ULL,216646957ULL},
		{ 1ULL,424591189ULL,1003159306ULL,1911076207ULL,2045675353ULL,1227931009ULL,1919393464ULL,155474929ULL,518277262ULL,118911409ULL,80613451ULL,528099832ULL,339131815ULL,563048662ULL,1784772466ULL,2083404574ULL,302749087ULL,1012593034ULL,1787118595ULL,987251272ULL,294603064ULL,2144030140ULL,260292436ULL,1862593549ULL,599969860ULL,1253561788ULL,1677941752ULL,1619354071ULL,1270183609ULL,893021566ULL,1677014089ULL,501706801ULL},
		{ 1ULL,210843040ULL,1228916023ULL,1017483766ULL,1561646362ULL,1606026670ULL,764971363ULL,73422361ULL,499383703ULL,1637457181ULL,492901246ULL,1697699494ULL,755836393ULL,1100846926ULL,2005827727ULL,854587087ULL,2119291921ULL,1095112225ULL,1554713296ULL,718973032ULL,1278847888ULL,2040801448ULL,1580912197ULL,424266418ULL,963374608ULL,1294653277ULL,1769360404ULL,1688794414ULL,1636284979ULL,1605406927ULL,204423346ULL,1636632718ULL},
		{ 1ULL,566549329ULL,1761804235ULL,830296516ULL,781918015ULL,1908415615ULL,1751331013ULL,40163332ULL,1830950671ULL,724860829ULL,2056631389ULL,1317437608ULL,2089220989ULL,1739796649ULL,156977629ULL,365854993ULL,184785544ULL,1720606414ULL,1188023044ULL,181218922ULL,569301730ULL,1072091953ULL,542897182ULL,593260111ULL,1556366704ULL,1120371469ULL,1066562152ULL,1410311230ULL,566145355ULL,780546613ULL,1052185504ULL,195784390ULL},
		{ 1ULL,730155415ULL,927030997ULL,880356631ULL,963694168ULL,1955666011ULL,1781280952ULL,1299434806ULL,1233838864ULL,167395729ULL,658476988ULL,2043554560ULL,1349796943ULL,1119812503ULL,1344654268ULL,1590665242ULL,207904591ULL,859184938ULL,1362614689ULL,1426583842ULL,1261614826ULL,219367078ULL,1196419747ULL,159106633ULL,658526563ULL,1513865641ULL,613900969ULL,1489595701ULL,1414293565ULL,1288859164ULL,535969462ULL,125189686ULL}
	};

__device__ __inline__ uint64_t LCGInitBit(uint64_t k)
{

	uint32_t	i = 0;
	uint64_t	q = (1ULL);// << (k & 0x1F);

	for (i = 0; i < 11; i++)
	{
		q = LCGStep(q,BCN_AUX_R[i][k & 0x1F]);
		k = k >> 5;
	}

	return LCGStep(q,LCG_a);
}


/*!
 ---------------------------------------------
	Function: seedlcg

	INPUTS
		s:	64 bit  integer type containing
			the seed value for the lcg generator in
			the range [0,2^31 + 1]
	OUTPUTS
		nil
 ---------------------------------------------
	NOTES:
 --------------------------------------------- 
*/
__device__ __inline__ uint64_t seedlcg(uint64_t s)
{
	return  LCGInitBit(s);
}


__device__ __inline__ void seedCombined(uint64_t count, uint64_t count1, uint64_t* seed, uint64_t* lcgseed)
{
	*seed=BarrettInitBit(count);
	*lcgseed=LCGInitBit(count1);
}




/*!
 ---------------------------------------------
	Function: randlcgSimple_increment

	INPUTS
		z:		pointer to 64 bit signed integer
				containing the k'th iterate beyond
				the seed iterate
	OUTPUTS
		r:		64 bit signed integer type holding
				the value of z_k+1
 ---------------------------------------------
	COMPUTES:
		z_k+1 = 39373 z_k mod 2^31 + 1
		r = z_k+1
	NOTES:
		Updates input z_k to z_k+1
 --------------------------------------------- 
*/
__device__ __inline__ uint64_t randlcgSimple_increment( uint64_t *z )
{
	return *z = (LCG_a * *z) % LCG_m;
}

/*!
 ---------------------------------------------
	Function: randCombined_increment

	INPUTS
				pointers to 64 bit signed integer
				containing the k'th iterate beyond
				the seed iterate
	OUTPUTS
		rnd:	64 bit uint64_t type
 ---------------------------------------------
	COMPUTES:
		rnd = LCG()-BCN() mod LCG_m - 1
 --------------------------------------------- 
*/
__device__ __inline__ uint64_t randCombined_increment(uint64_t *SeedBCN_z_k, uint64_t *SeedLCG_z_k)
{
	uint64_t qhi, qlo, r2lo;
	barrett_step_opt(*SeedBCN_z_k);
	return (int64_t)(randlcgSimple_increment(SeedLCG_z_k) -  *SeedBCN_z_k) & ULL(0x7FFFFFFF); //800000007FFFFFFF
}









