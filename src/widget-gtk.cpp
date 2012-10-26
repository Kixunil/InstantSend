#include <cstdio>
#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/treeview.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/notebook.h>
#include <gtkmm/main.h>
#include <gtkmm/box.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/stock.h>
#include <gtkmm/image.h>
#include <glibmm/markup.h>
#include <stdint.h>
#include <dbus/dbus.h>

#include <vector>
#include <map>
#include <memory>

#include "json.h"
#include "sysapi.h"

using namespace Gtk;
using namespace std;
using namespace Glib;

ustring intToStr(int value) {
	std::ostringstream ssIn;
	ssIn << value;
	Glib::ustring strOut = ssIn.str();

	return strOut;
}

class FileInfoBaseRenderer {
	public:
		virtual Widget &getWidget() = 0;
		virtual void updateData() = 0;
};

class FileInfoItem {
	private:
		ustring fileName;
		ustring peerName;
		char status;
		char direction;
		uint64_t fileSize, bytes;
		FileInfoBaseRenderer *renderer;
	public:
		inline FileInfoItem() {
			direction = 0;
			bytes = 0;
			renderer = NULL;
		}
		inline FileInfoItem(char dir) {
			direction = dir;
		}

		inline void setDirection(char d) {
			direction = d;
		}

		inline ustring getFileName() {
			return fileName;
		}

		inline ustring getPeerName() {
			return peerName;
		}

		inline char getStatus() {
			return status;
		}

		inline char getProgress() {
			return (100 * bytes) / fileSize;
		}

		inline char getDirection() {
			return direction;
		}

		inline void setFileName(const ustring &fileName) {
			this->fileName = fileName;
			if(renderer) renderer->updateData();
		}


		inline void setPeerName(const ustring &peerName) {
			this->peerName = peerName;
			if(renderer) renderer->updateData();
		}

		inline void setSize(uint64_t size) {
			fileSize = size;
			if(renderer) renderer->updateData();
		}

		inline void setStatus(char status) {
			this->status = status;
			if(renderer) renderer->updateData();
		}

		inline void setBytes(uint64_t b) {
			this->bytes = b;
			if(renderer) renderer->updateData();
		}

		inline void setRenderer(auto_ptr<FileInfoBaseRenderer> renderer) {
			this->renderer = renderer.release();
		}

		inline void update() {
			renderer->updateData();
		}

		inline void removeRenderer(Container &container) {
			container.remove(renderer->getWidget());
		}
};

class SimpleFileInfoRenderer : public FileInfoBaseRenderer {
	private:
		FileInfoItem *finfo;

	public:
		Label fileName, peerName;
		ProgressBar progress;
		VBox mainBox;
		VBox textInfo;
		HBox info;
		HBox progressContainer;
		Button pause, cancel;
		Image icon;
		SimpleFileInfoRenderer(FileInfoItem *fileInfo) : pause(Stock::MEDIA_PAUSE), cancel(Stock::CANCEL), icon(fileInfo->getDirection()?Stock::GOTO_TOP:Stock::GOTO_BOTTOM, IconSize(ICON_SIZE_DND)) {
			finfo = fileInfo;
			textInfo.pack_start(fileName, PACK_EXPAND_WIDGET);
			textInfo.pack_start(peerName, PACK_EXPAND_WIDGET);
			progressContainer.pack_start(progress, PACK_EXPAND_WIDGET);
			progressContainer.pack_start(pause, PACK_SHRINK);
			progressContainer.pack_start(cancel, PACK_SHRINK);
			info.pack_start(textInfo, PACK_EXPAND_WIDGET);
			info.pack_start(icon, PACK_SHRINK);
			mainBox.pack_start(info, PACK_SHRINK);
			mainBox.pack_start(progressContainer, PACK_SHRINK);
			updateData();
		}

		void updateData() {
			fileName.set_markup("<b>" + Markup::escape_text(finfo->getFileName()) + "</b>");
			fileName.set_justify(JUSTIFY_LEFT);
			peerName.set_markup("<small>" + Markup::escape_text(finfo->getPeerName()) + "</small>");
			peerName.set_justify(JUSTIFY_LEFT);
			switch(finfo->getStatus()) {
				case 1:
					progress.set_text("Connecting");
					progress.pulse();
					break;
				case 2:
					progress.set_text("Waiting for accept");
					progress.pulse();
					break;
				case 3:
					progress.set_text(ustring("Sending (") + intToStr(finfo->getProgress()) + "%)");
					progress.set_fraction((float)finfo->getProgress() / 100);
					break;
				case 4:
					progress.set_text(ustring("Receiving (") + intToStr(finfo->getProgress()) + "%)");
					progress.set_fraction((float)finfo->getProgress() / 100);
					break;
				case 5:
					progress.set_text(ustring("Paused (") + intToStr(finfo->getProgress()) + "%)");
					progress.set_fraction((float)finfo->getProgress() / 100);
					break;
			}
			/*mainBox.queue_draw();*/
			printf("Data updated, progress: %d\n", finfo->getProgress());
		}

		Widget &getWidget() {
			return mainBox;
		}
};

class instSendWidget : public Window {
	public:
		instSendWidget();

		void addFile(unsigned int id, const ustring &fileName, const ustring &machineId, uint64_t fileSize, char direction) {
			FileInfoItem &finfo = *new FileInfoItem();
			auto_ptr<SimpleFileInfoRenderer> finfoRenderer;

			finfo.setDirection(direction);
			finfo.setFileName(fileName);
			finfo.setPeerName(machineId);
			finfo.setSize(fileSize);
			finfo.setStatus(4);
			files[id] = &finfo;
			finfoRenderer = auto_ptr<SimpleFileInfoRenderer>(new SimpleFileInfoRenderer(&finfo));
			allBox.pack_start(finfoRenderer->getWidget(), PACK_SHRINK);
			finfo.setRenderer(auto_ptr<FileInfoBaseRenderer>(finfoRenderer.release()));

			show_all_children();
		}

		inline void updateBytes(unsigned int id, uint64_t bytes) {
			if(files.count(id))
			files[id]->setBytes(bytes);
		}

		virtual ~instSendWidget() {}

	protected:

		ScrolledWindow scrollWindowAll, scrollWindowRecv, scrollWindowSend;
		VBox allBox;

		std::map<unsigned int, FileInfoItem *> files;
		unsigned int nextid;

		Notebook notebook;

		DBusError dberr;
		DBusConnection *dbconn;


		bool on_timeout() {
			dbus_connection_read_write_dispatch (dbconn, 10);
			/*std::map<unsigned int, FileInfoItem>::iterator it = files.begin(), e = files.end();
			for(; it != e; ++it) {
				it->second.update();
			}*/
			return true;
		}
};

static DBusHandlerResult
filter_func (DBusConnection *connection,
             DBusMessage *message,
             void *user_data)
{
	instSendWidget &isWidget = *(instSendWidget *)user_data;
	dbus_bool_t handled = FALSE;

	uint64_t fileSize;
	uint32_t fileId;
	uint64_t transferredBytes;
	char *fileName = NULL, *machineId = NULL;
	uint8_t direction = 0;

	if (dbus_message_is_signal (message, "sk.pixelcomp.instantsend", "onBegin")) {
		DBusError dberr;

		dbus_error_init (&dberr);
		dbus_message_get_args (message, &dberr, DBUS_TYPE_UINT32, &fileId, DBUS_TYPE_STRING, &fileName, DBUS_TYPE_STRING, &machineId, DBUS_TYPE_UINT64, &fileSize, DBUS_TYPE_BYTE, &direction, DBUS_TYPE_INVALID);
		if (dbus_error_is_set (&dberr)) {
			fprintf (stderr, "Error getting message args: %s", dberr.message);
			dbus_error_free (&dberr);
		} else {
			isWidget.addFile(fileId, ustring(fileName), ustring(machineId), fileSize, direction);

			handled = TRUE;
		}
	} else if(dbus_message_is_signal (message, "sk.pixelcomp.instantsend", "onUpdate")) {
		DBusError dberr;

		dbus_error_init (&dberr);
		dbus_message_get_args (message, &dberr, DBUS_TYPE_UINT32, &fileId, DBUS_TYPE_UINT64, &transferredBytes, DBUS_TYPE_INVALID);
		if (dbus_error_is_set (&dberr)) {
			fprintf (stderr, "Error getting message args: %s", dberr.message);
			dbus_error_free (&dberr);
		} else {
			isWidget.updateBytes(fileId, transferredBytes);
//			printf("Received onUpdate signal, file: %u, bytes: %lu", fileId, transferredBytes);

			handled = TRUE;
		}
	}

	return (handled ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

instSendWidget::instSendWidget() {
	try {
		auto_ptr<jsonComponent_t> cfgptr;
		try {
			string cfgfile = combinePath(getUserDir(), "widget-gtk.cfg");
			cfgptr = cfgReadFile(cfgfile.c_str());
		} catch(fileNotAccesible &e) {
			string cfgfile = combinePath(getSystemCfgDir(), "widget-gtk.cfg");
			cfgptr = cfgReadFile(cfgfile.c_str());
		}
		jsonObj_t &cfg = dynamic_cast<jsonObj_t &>(*cfgptr.get());

		set_icon_from_file(dynamic_cast<jsonStr_t &>(cfg.gie("icon")).getVal());
	} catch(exception &e) {
		printf("Excepion occured during loading of icon: %s\n", e.what());
	}

	set_title("Files");
	set_default_size(400, 200);

	dbus_error_init (&dberr);
	dbconn = dbus_bus_get (DBUS_BUS_SESSION, &dberr);
	if (dbus_error_is_set (&dberr)) {
		fprintf (stderr, "getting session bus failed: %s\n", dberr.message);
		dbus_error_free (&dberr);
		return;
	}

	dbus_bus_request_name (dbconn, "sk.pixelcomp.instantsend",
	                       DBUS_NAME_FLAG_REPLACE_EXISTING, &dberr);
	if (dbus_error_is_set (&dberr)) {
		fprintf (stderr, "requesting name failed: %s\n", dberr.message);
		dbus_error_free (&dberr);
		return;
	}

	if (!dbus_connection_add_filter (dbconn, filter_func, this, NULL))
		return;

	dbus_bus_add_match (dbconn,
	                    "type='signal',interface='sk.pixelcomp.instantsend'",
	                    &dberr);

	if (dbus_error_is_set (&dberr)) {
		fprintf (stderr, "Could not match: %s", dberr.message);
		dbus_error_free (&dberr);
		return;
	}


	add(notebook);
	notebook.append_page(scrollWindowAll, "All files");
	notebook.append_page(scrollWindowRecv, "Incoming files");
	notebook.append_page(scrollWindowSend, "Outgoing files");

	scrollWindowAll.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrollWindowRecv.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrollWindowSend.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);

	//scrollWindowRecv.add(recvView);
	//scrollWindowSend.add(sendView);
	
	FileInfoItem &finfo = *new FileInfoItem();
	auto_ptr<SimpleFileInfoRenderer> finfoRenderer;

/*
	finfo.setDirection(1);
	finfo.setFileName("Test file 1");
	finfo.setPeerName("Machine 1");
	finfo.setStatus(1);
	files[1] = &finfo;
	finfoRenderer = auto_ptr<SimpleFileInfoRenderer>(new SimpleFileInfoRenderer(&finfo));
	allBox.pack_start(finfoRenderer->getWidget(), PACK_SHRINK);
	finfo.setRenderer(auto_ptr<FileInfoBaseRenderer>(finfoRenderer.release()));
	finfo.setDirection(1);
	finfo.setFileName("Test file 2");
	finfo.setPeerName("Machine 2");
	finfo.setStatus(2);
	finfoRenderer = auto_ptr<SimpleFileInfoRenderer>(new SimpleFileInfoRenderer(&files[addFile(finfo)]));
	allBox.pack_start(finfoRenderer->getWidget(), PACK_SHRINK);
	finfo.setRenderer(auto_ptr<FileInfoBaseRenderer>(finfoRenderer.release()));

	finfo.setDirection(1);
	finfo.setFileName("Test file 3");
	finfo.setPeerName("Machine 3");
	finfo.setStatus(3);
	finfoRenderer = auto_ptr<SimpleFileInfoRenderer>(new SimpleFileInfoRenderer(&files[addFile(finfo)]));
	allBox.pack_start(finfoRenderer->getWidget(), PACK_SHRINK);
	finfo.setRenderer(auto_ptr<FileInfoBaseRenderer>(finfoRenderer.release()));

	finfo.setDirection(1);
	finfo.setFileName("Test file 4");
	finfo.setPeerName("Machine 4");
	finfo.setStatus(4);
	finfoRenderer = auto_ptr<SimpleFileInfoRenderer>(new SimpleFileInfoRenderer(&files[addFile(finfo)]));
	allBox.pack_start(finfoRenderer->getWidget(), PACK_SHRINK);
	finfo.setRenderer(auto_ptr<FileInfoBaseRenderer>(finfoRenderer.release()));
*/
	scrollWindowAll.add(allBox);

	show_all_children();

	signal_timeout().connect(sigc::mem_fun(*this, &instSendWidget::on_timeout), 100);
}

int main(int argc, char *argv[]) {
	Main app(argc, argv, "sk.pixelcomp.instantsend.widget");
	instSendWidget window;
	app.run(window);
	return 0;
}
