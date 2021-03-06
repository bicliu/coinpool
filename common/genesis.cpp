#include "utils.h"

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>
#include "arith_uint256.h"
#include "chainparamsseeds.h"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <cstdio>
#include <string>
#include "utiltime.h"
#include <random>
#include <cmath>
#include <iomanip>
#include <util.h>
#include <new>
#include <chrono>
#include <thread>
#include <mutex>
#include "random.h"

#include <sys/time.h>

using namespace std;

typedef uint32_t uint;
typedef CBlockHeader ch;
typedef long long ll;

static std::mutex mtx;

//test cnt 1000 times time
/*int64_t getCurrentTime()  
{      
   struct timeval tv;      
   gettimeofday(&tv,NULL);   
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;      
}*/

// find a genesis in about 10-20 mins
void _get(const ch * const pblock, const arith_uint256 hashTarget, const int index, int * result)
{
    uint256 hash;
    ch *pb = new ch(*pblock);
    int64_t timeoffset = pblock->nTime - GetTime();

	//std::cout<< "_get " << index <<" start nonce = "<<pb->nNonce.ToString()<<std::endl;
	
    for (int cnt = 0; true; ++cnt)
    {
        uint256 hash = pb->GetHash();

		//std::cout<< "_get " << index <<" hex hash = "<<hash.GetHex()<<std::endl;
		//std::cout<< "_get " << index <<" pb nonce = "<<pb->nNonce.ToString()<<std::endl;
		
        if (UintToArith256(hash) <= hashTarget)
        {
            (*result)++;
			cout << "Task " << index << " get target." << endl;
            std::lock_guard<std::mutex> guard(mtx);
            std::cout << "\n\t\t----------------------------------------\t" << std::endl;
            std::cout << "\t" << pb->ToString();
            std::cout << "\n\t\t----------------------------------------\t" << std::endl;
        }
        if(*result > 0)
            break;

        pb->nNonce = ArithToUint256(UintToArith256(pb->nNonce) + 1);

        if (cnt > 1e3)
        {
            pb->nTime = GetTime() + timeoffset;
            cnt = 0;
            //std::cout<< "_get " << index << " time " << pb->nTime << "\r";
        }
		/*if (tcnt !=0 and tcnt % 1000 == 0)
        {
            std::cout<<"_get "<<index<<" cryptohello tcnt = "<<tcnt<<" time = "<<getCurrentTime()<<" ms"<<std::endl;       
        }*/

    }
    
    delete pb;

    // stop while found one
    //assert(0);
    return;
}

static void findGenesis(CBlockHeader *pb)
{
    arith_uint256 hashTarget = arith_uint256().SetCompact(pb->nBits);
    std::cout << " finding genesis using target " << hashTarget.ToString() << std::endl;

    std::vector<std::thread> threads;
	int icpu = std::min(GetNumCores(), 100);
    int iresult = 0;
	cout << "Get " << icpu << " cpus to use" << endl;
    //for (int i = 0; i < std::min(GetNumCores(), 100); ++i)
    for (int i = 0; i < icpu; ++i)
    {
        if (i >= 0)
        {
		// Randomise nonce
        	arith_uint256 nonce = UintToArith256(GetRandHash());
        	// Clear the top and bottom 16 bits (for local use as thread flags and counters)
        	nonce <<= 32;
        	nonce >>= 16;
        	pb->nNonce = ArithToUint256(nonce);
			//std::cout<<"i = "<<i<<"    nNonce = "<<pb->nNonce.ToString()<<std::endl;	
        }
        threads.push_back(std::thread(_get, pb, hashTarget, i, &iresult));
		//MilliSleep(100);
    }

    for (auto &t : threads)
    {
        t.join();
    }
}

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint256 nNonce, uint32_t nBits, int32_t nVersion, const CAmount &genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = CAmount(genesisReward);
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashClaimTrie = uint256S("0x1");
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
 *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
 *   vMerkleTree: e0028e
 */
/*static CBlock CreateGenesisBlock(uint32_t nTime, uint256 nNonce, uint32_t nBits, int32_t nVersion, const int64_t& genesisReward)
{
    const char* pszTimestamp = "ulord hold value testnet.";
    const CScript genesisOutputScript = CScript() << ParseHex("034c73d75f59061a08032b68369e5034390abc5215b3df79be01fb4319173a88f8") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock CreateGenesisBlock1(uint32_t nTime, uint256 nNonce, uint32_t nBits, int32_t nVersion, const int64_t& genesisReward)                                                                                                                
{
    const char* pszTimestamp = "Change the World with Us. 22/May/2018, 00:00:00, GMT";
    const CScript genesisOutputScript = CScript() << ParseHex("041c508f27e982c369486c0f1a42779208b3f5dc96c21a2af6004cb18d1529f42182425db1e1632dc6e73ff687592e148569022cee52b4b4eb10e8bb11bd927ec0") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}*/

void GenesisLookForHelp()
{
    cout << "Command \"genesis\" example :" << endl << endl
        << "genesis unixTime \"stamp\" ..." << endl << endl;
}

void GenesisLookFor(int argc, char* argv[])
{
    if(argc < cmdindex+3)
    {
        GenesisLookForHelp();
        return;
    }

    arith_uint256 nTempBit =  UintToArith256( Params().GetConsensus().powLimit);
    CBlock genesis;
    arith_uint256 a;
    CScript genesisOutputScript;

    if (Params().NetworkIDString() == CBaseChainParams::MAIN)
    {
        //genesis =CreateGenesisBlock1(1526947200, uint256S("0x01"), nTempBit.GetCompact(), 1, Params().GetConsensus().genesisReward);
        genesisOutputScript = CScript() << ParseHex("041c508f27e982c369486c0f1a42779208b3f5dc96c21a2af6004cb18d1529f42182425db1e1632dc6e73ff687592e148569022cee52b4b4eb10e8bb11bd927ec0") << OP_CHECKSIG;
        genesis = CreateGenesisBlock(argv[cmdindex+2], genesisOutputScript, atoi(argv[cmdindex+1]), uint256S("0x01"), nTempBit.GetCompact(), 1, Params().GetConsensus().genesisReward);
        a = arith_uint256("0x000009b173000000000000000000000000000000000000000000000000000000");
    }
    else if(Params().NetworkIDString() == CBaseChainParams::TESTNET)
    {
        //genesis = CreateGenesisBlock(1526947200, uint256S("0x01"), nTempBit.GetCompact(), 1,  1 * COIN);
        genesisOutputScript = CScript() << ParseHex("034c73d75f59061a08032b68369e5034390abc5215b3df79be01fb4319173a88f8") << OP_CHECKSIG;
        genesis = CreateGenesisBlock(argv[cmdindex+2], genesisOutputScript, atoi(argv[cmdindex+1]), uint256S("0x01"), nTempBit.GetCompact(), 1, 1 * COIN);
        a = arith_uint256("0x000fffffff000000000000000000000000000000000000000000000000000000");
    }
    else
        return;

    cout << "\tpow:\t" << a.GetCompact()  << " "<< nTempBit.GetCompact() << endl;
    cout <<"hashMerkleRoot: " << genesis.hashMerkleRoot.ToString() << endl;
    //cout << endl << genesis.ToString() << endl;
    
    findGenesis(&genesis);

    //for regtest
    if (Params().NetworkIDString() == CBaseChainParams::MAIN)
    {
        cout << endl << "Looking for regtest: " << endl;
        genesis = CreateGenesisBlock(argv[cmdindex+2], genesisOutputScript, atoi(argv[cmdindex+1]), uint256S("0x01"), 0x200f0f0f, 1, 1 * COIN);
        findGenesis(&genesis);
    }

    cout << "create end." << endl;

    return;
}

void GetTimeHandle(int argc, char* argv[])
{
    int64_t tnow = GetTime();
		int64_t tmnow = GetTimeMicros();
		cout << "Time is " << DateTimeStrFormat("%Y-%m-%d %H:%M:%S", tnow) << endl
			<< "<" << tnow << ">,<" << tmnow << ">,<" << GetTimeMillis() << ">" << endl;;
}

void GetRpcHelp()
{
    cout << "Command \"getrpc\" example :" << endl << endl
        << "getrpc rpcuser rpcpassword ..." << endl
		<< "getrpc \"thgy\" \"123\""<< endl;
}

void GetRPCAuthorization(int argc, char* argv[])
{
    if(argc < cmdindex+3)
    {
        GetRpcHelp();
        return;
    }
    string rpcuser = argv[cmdindex + 1];
    string rpcpsw = argv[cmdindex + 2];

    string strRPCUserColonPass = rpcuser + ":" + rpcpsw;

    cout << "rpcuser=" << rpcuser << ", rpcpassword=" << rpcpsw
         << endl << "Basic " << EncodeBase64(strRPCUserColonPass) << endl;
}
