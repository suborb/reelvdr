#include "conlogger.h"
#include "chain.h"
#include "shellprocess.h"
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

using std::vector;
using std::string;
using std::copy;
using std::stringstream;

using namespace proctools;

struct recording
{
	string path;
	string temp;
	string data;
	string video;
	string audio;
	string movie;
};

recording recs[2];

class procchain_dvd: public chain
{
public:
	procchain_dvd();

protected:
	virtual bool initialize();
	virtual void process_line(const string& line);
	virtual void finished(const process* process);

	void prepare_recording();

private:
	int m_index;
};

procchain_dvd::procchain_dvd():
		chain("DVD"),
		m_index(0)
{
}

bool procchain_dvd::initialize()
{
	shellprocess *proc;

	recs[0].path = "/video0/Dragon_Ball_Z/001_Ein_ausserirdischer_Krieger/2006-01-31.19.10.50.50.rec";
	recs[1].path = "/video0/Dragon_Ball_Z/002_Son_Gokus_Vergangenheit/2006-01-31.19.35.50.50.rec";

	make_dir("temp");
	make_dir("data");

	for (int i = 0; i < 2; ++i) {
		stringstream stream;

		stream << "temp/vdrsync." << i;
		recs[i].temp = stream.str();
		stream.str("");
		stream.clear();

		stream << "data/vdrsync." << i;
		recs[i].data = stream.str();
		stream.str("");
		stream.clear();

		stream << recs[i].temp << "/vdrsync.mpv";
		recs[i].video = stream.str();
		stream.str("");
		stream.clear();

		stream << recs[i].temp << "/vdrsync0.mpa";
		recs[i].audio = stream.str();
		stream.str("");
		stream.clear();

		stream << recs[i].temp << "/movie.mpg";
		recs[i].movie = stream.str();
		stream.str("");
		stream.clear();

		make_dir(recs[i].temp);
		make_dir(recs[i].data);
		make_fifo(recs[i].video);
		make_fifo(recs[i].audio);
		make_fifo(recs[i].movie);
	}

	make_dir("data/dvdauthor");

	proc = new shellprocess("author", "vdrburn-dvd.sh author");
	proc->put_environment("DVDAUTHOR_PATH", "data/dvdauthor");
	add_process(proc);

	prepare_recording();

	return true;
}

void procchain_dvd::process_line(const std::string& message)
{
}

void procchain_dvd::finished(const process* process)
{
	if (process->name() == "mplex" && process->result() == 0) {
		if (m_index < 2)
			prepare_recording();
	}
}

void procchain_dvd::prepare_recording()
{
	process *proc = new shellprocess("demux", "vdrburn-dvd.sh demux");
	proc->put_environment("RECORDING_PATH", recs[m_index].path);
	proc->put_environment("VIDEO_FILE", recs[m_index].video);
	proc->put_environment("MPEG_PATH", recs[m_index].temp);
	add_process(proc);

	proc = new shellprocess("mplex", "vdrburn-dvd.sh mplex");
	proc->put_environment("VIDEO_FILE", recs[m_index].video);
	proc->put_environment("AUDIO_FILES", recs[m_index].audio);
	proc->put_environment("MOVIE_FILE", recs[m_index].movie);
	add_process(proc);

	++m_index;
}

int main(int argc, char *argv[])
{
	vector<string> args(argc - 1);
	copy(argv + 1, argv + argc, args.begin());

	conlogger::start();

	logger::info("Starte Job");

	procchain_dvd chain;
	if (chain.run()) {
	}
}
