#include <cstdio>
#include <gtkmm/action.h>
#include <gtkmm/actiongroup.h>
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
#include <gtkmm/statusicon.h>
#include <gtkmm/uimanager.h>
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

class dialogControl {
	public:
		virtual void show() = 0;
		virtual void hide() = 0;
		virtual void togle() = 0;
		virtual void quit() = 0;
		virtual bool isVisible() = 0;
		virtual unsigned int recvInProgress() = 0;
		virtual unsigned int sendInProgress() = 0;
		virtual unsigned int recvPending() = 0;
		virtual unsigned int sendPending() = 0;
};

class trayIcon {
	protected:
		dialogControl *dlg;
		Glib::RefPtr<ActionGroup> actions;
	public:
		inline trayIcon(dialogControl *dialog) : dlg(dialog), actions(Gtk::ActionGroup::create()) {
			actions->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT), sigc::mem_fun(*dlg, &dialogControl::quit));
		}

		virtual void show() = 0;
		virtual void hide() = 0;
		virtual void timer() = 0;
		virtual void statusChanged() = 0;
};

#define LOAD_ICON(PATH) Gdk::Pixbuf::create_from_file(combinePath(getSystemDataDir(), PATH) /*, statusIcon->get_size(), statusIcon->get_size()*/)

class gtkTrayIcon : public trayIcon {
	private:
		unsigned int animPtr, cnt;
		bool animating, excmark, animem;
		Glib::RefPtr<Gdk::Pixbuf> mainIcon, dlStaticIcon, ulStaticIcon, udStaticIcon, excmarkIcon;
		vector<Glib::RefPtr<Gdk::Pixbuf> > dlAnim, ulAnim;
		Glib::RefPtr<Gtk::StatusIcon> statusIcon;
		Glib::RefPtr<Gtk::UIManager> uimanager;
		Gtk::Menu* popupmenu;
	protected:
		void on_icon_activate() {
			dlg->togle();
			show();
		}

		void on_popup_menu(guint button, guint32 activate_time) {
			statusIcon->popup_menu_at_position(*popupmenu, button, activate_time);
		}
	public:
		gtkTrayIcon(dialogControl *dlg) : trayIcon(dlg), uimanager(Gtk::UIManager::create()), animPtr(0) {
			try {
				mainIcon = LOAD_ICON("icon_64.png");
				dlStaticIcon = LOAD_ICON("icon_download_64.png");
				ulStaticIcon = LOAD_ICON("icon_upload_64.png");
				udStaticIcon = LOAD_ICON("icon_uploaddownload_64.png");
				excmarkIcon = LOAD_ICON("icon_exclamationmark_64.png");

				dlAnim.push_back(LOAD_ICON("icon_download_1_64.png"));
				dlAnim.push_back(LOAD_ICON("icon_download_2_64.png"));
				dlAnim.push_back(LOAD_ICON("icon_download_3_64.png"));

				ulAnim.push_back(LOAD_ICON("icon_upload_1_64.png"));
				ulAnim.push_back(LOAD_ICON("icon_upload_2_64.png"));
				ulAnim.push_back(LOAD_ICON("icon_upload_3_64.png"));

				animating = false;
				statusIcon = StatusIcon::create(mainIcon);
				statusIcon->signal_activate().connect(sigc::mem_fun(*this, &gtkTrayIcon::on_icon_activate));

				Glib::ustring ui_info =
					"<ui>"
					"  <popup name='TrayIconPopup'>"
					"    <menuitem action='Quit'/>"
					"  </popup>"
					"</ui>";
				uimanager->add_ui_from_string(ui_info);

				uimanager->insert_action_group(actions);
				popupmenu = dynamic_cast<Gtk::Menu*>(uimanager->get_widget("/TrayIconPopup"));
				statusIcon->signal_popup_menu().connect(sigc::mem_fun(*this, &gtkTrayIcon::on_popup_menu));

			} catch(Glib::FileError &e) {
				printf("Error: %s\n", e.what().c_str());
			}
		}

		void show() {
			statusIcon->set_visible();
		}

		void hide() {
			statusIcon->set_visible(false);
		}

		void timer() {
			if(animating) {
				animPtr = (animPtr + 1) % 3;
				if(dlg->recvInProgress()) statusIcon->set(dlAnim[animPtr]); else statusIcon->set(ulAnim[animPtr]);
			}

			if(++cnt > 10) {
				cnt = 0;
				if(excmark) {
					bool rpe = dlg->recvPending(), rip = dlg->recvInProgress(), spe = dlg->sendPending(), sip = dlg->sendInProgress();
					if((rpe || rip) && (spe || sip)) {
						statusIcon->set(ulStaticIcon);
						animating = false;
					} else {
						if(rip) {
							animating = true;
							statusIcon->set(dlAnim[animPtr]);
						} else
						if(sip) {
							animating = true;
							statusIcon->set(dlAnim[animPtr]);
						} else
						if(rpe) {
							animating = false;
							statusIcon->set(dlStaticIcon);
						} else
						if(spe) {
							animating = false;
							statusIcon->set(ulStaticIcon);
						}
					}
				} else {
					if(animem) {
						statusIcon->set(excmarkIcon);
					}
				}
				excmark = !excmark;
			}
		}

		void statusChanged() {
			bool rpe = dlg->recvPending(), rip = dlg->recvInProgress(), spe = dlg->sendPending(), sip = dlg->sendInProgress();
			animem = rpe;
			excmark = false;
			if((rpe || rip) && (spe || sip)) {
				statusIcon->set(ulStaticIcon);
				animating = false;
			} else {
				if(rip) {
					animating = true;
					statusIcon->set(dlAnim[animPtr]);
				} else
				if(sip) {
					animating = true;
					statusIcon->set(dlAnim[animPtr]);
				} else
				if(rpe) {
					animating = false;
					statusIcon->set(dlStaticIcon);
				} else
				if(spe) {
					animating = false;
					statusIcon->set(ulStaticIcon);
				} else {
					animating = false;
					statusIcon->set(mainIcon);	
				}
			
			}

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

Main *appPtr;

class instSendWidget : public Window, dialogControl {
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
			++recvInProgressCount;
			trIcon.statusChanged();
		}

		inline void updateBytes(unsigned int id, uint64_t bytes) {
			if(files.count(id)) {
				files[id]->setBytes(bytes);
				if(files[id]->getProgress() == 100) {
					--recvInProgressCount;
					trIcon.statusChanged();
				}
			}
		}

		virtual ~instSendWidget() {}

		virtual void show() {
			Window::show();
		}

		virtual void hide() {
			Window::hide();
		}

		virtual void togle() {
			if(Window::is_visible()) Window::hide(); else Window::show();
		}

		virtual void quit() {
			appPtr->quit();
		}

		virtual bool isVisible() {
			return Window::is_visible();
		}

		virtual unsigned int recvInProgress() {
			return recvInProgressCount;
		}

		virtual unsigned int sendInProgress() {
			return 0;
		}

		virtual unsigned int recvPending() {
			return 0;
		}

		virtual unsigned int sendPending() {
			return 0;
		}

	protected:

		ScrolledWindow scrollWindowAll, scrollWindowRecv, scrollWindowSend;
		VBox allBox;

		std::map<unsigned int, FileInfoItem *> files;
		unsigned int nextid, recvInProgressCount;

		Notebook notebook;

		DBusError dberr;
		DBusConnection *dbconn;
		gtkTrayIcon trIcon;


		bool on_timeout() {
			dbus_connection_read_write_dispatch (dbconn, 10);
			/*std::map<unsigned int, FileInfoItem>::iterator it = files.begin(), e = files.end();
			for(; it != e; ++it) {
				it->second.update();
			}*/
			return true;
		}

		bool on_icon_timeout() {
			trIcon.timer();
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

instSendWidget::instSendWidget() : trIcon(this), recvInProgressCount(0) {
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
		try {
			set_icon_from_file(combinePath(getSystemDataDir(), "icon_32.png"));
		}
		catch(exception &e) {
			printf("Excepion occured during loading of icon: %s\n", e.what());
		}
		catch(...) {
		}
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
	signal_timeout().connect(sigc::mem_fun(*this, &instSendWidget::on_icon_timeout), 200);
}

int main(int argc, char *argv[]) {
	Main app(argc, argv, "sk.pixelcomp.instantsend.widget");
	appPtr = &app;
	instSendWidget window;
	app.run();
	return 0;
}
