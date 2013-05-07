#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
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
#include <locale.h>
#include <libintl.h>

#include <vector>
#include <map>
#include <set>
#include <memory>

#include "json.h"
#include "sysapi.h"
#include "multithread.h"

#define _(str) gettext(str)

using namespace Gtk;
using namespace std;
using namespace Glib;

ustring intToStr(int value) {
	std::ostringstream ssIn;
	ssIn << value;
	Glib::ustring strOut = ssIn.str();

	return strOut;
}

class observer_t {
	public:
		virtual void updateData() = 0;
};

class FileInfoItem {
	private:
		ustring fileName;
		ustring peerName;
		char status;
		char direction;
		uint64_t fileSize, bytes;
		set <observer_t *> observers;

	public:
		typedef set<observer_t *>::iterator obsiterator;

		inline FileInfoItem() {
			direction = 0;
			bytes = 0;
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
			update();
		}


		inline void setPeerName(const ustring &peerName) {
			this->peerName = peerName;
			update();
		}

		inline void setSize(uint64_t size) {
			fileSize = size;
		}

		inline uint64_t getSize() {
			return fileSize;
		}

		inline void setStatus(char status) {
			this->status = status;
			update();
		}

		inline void setBytes(uint64_t b) {
			this->bytes = b;
			update();
		}

		inline void addObserver(observer_t &observer) {
			this->observers.insert(&observer);
		}

		inline void removeObserver(observer_t &observer) {
			this->observers.erase(&observer);
		}

		inline void removeObserver(obsiterator &it) {
			this->observers.erase(it);
		}

		inline void clearObservers() {
			this->observers.clear();
		}

		inline obsiterator firstObserver() {
			return obsiterator(observers.begin());
		}

		inline obsiterator lastObserver() {
			return obsiterator(observers.end());
		}

		inline void update() {
			for(set<observer_t *>::iterator it = observers.begin(); it != observers.end(); ++it) {
				(*it)->updateData();
			}
		}
};

class dialogControl {
	public:
		virtual void show() = 0;
		virtual void hide() = 0;
		virtual void togle() = 0;
		virtual void quit() = 0;
		virtual void sendFiles() = 0;
		virtual void preferences() = 0;
		virtual void startServer() = 0;
		virtual void stopServer() = 0;
		virtual bool isVisible() = 0;
		virtual bool serverRunning() = 0;
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
			actions->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT, _("Quit")), sigc::mem_fun(*dlg, &dialogControl::quit));
			actions->add( Gtk::Action::create_with_icon_name("SendFiles", ustring("document-send"), ustring(_("Send files")), ustring()), sigc::mem_fun(*dlg, &dialogControl::sendFiles));
			actions->add( Gtk::Action::create("Preferences", Gtk::Stock::PREFERENCES, _("Preferences")), sigc::mem_fun(*dlg, &dialogControl::preferences));
			actions->add( Gtk::Action::create_with_icon_name("StartServer", ustring("system-run"), ustring(_("Start server")), ustring()), sigc::mem_fun(*dlg, &dialogControl::startServer));
			actions->add( Gtk::Action::create_with_icon_name("StopServer", ustring("stop"), ustring(_("Stop server")), ustring()), sigc::mem_fun(*dlg, &dialogControl::stopServer));

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
	protected:
		void on_icon_activate() {
			dlg->togle();
			show();
		}

		void on_popup_menu(guint button, guint32 activate_time) {
			Gtk::Menu& popupmenu = dynamic_cast<Gtk::Menu&>(*uimanager->get_widget((dlg->serverRunning())?"/TrayIconPopup_serverRunning":"/TrayIconPopup_serverStopped")); 
			statusIcon->popup_menu_at_position(popupmenu, button, activate_time);
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
					"  <popup name='TrayIconPopup_serverStopped'>"
					"    <menuitem action='SendFiles'/>"
					"    <menuitem action='StartServer'/>"
					"    <menuitem action='Preferences'/>"
					"    <menuitem action='Quit'/>"
					"  </popup>"
					"  <popup name='TrayIconPopup_serverRunning'>"
					"    <menuitem action='SendFiles'/>"
					"    <menuitem action='StopServer'/>"
					"    <menuitem action='Preferences'/>"
					"    <menuitem action='Quit'/>"
					"  </popup>"

					"</ui>";
				uimanager->add_ui_from_string(ui_info);

				uimanager->insert_action_group(actions);
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

class SimpleFileInfoRenderer : public observer_t {
	private:
		FileInfoItem *finfo;
		int lastStatus;

	public:
		Label fileName, peerName;
		ProgressBar progress;
		VBox mainBox;
		VBox textInfo;
		HBox info;
		HBox progressContainer, buttonContainer;
		Button pause, cancel, open, opendir, remove, deletedata;
		Image icon, iconpause, iconcancel, iconopen, icondir, icondelete, iconremove;
		SimpleFileInfoRenderer(FileInfoItem *fileInfo) : finfo(fileInfo), lastStatus(0), pause(_("Pause")), cancel(_("Cancel")), open(_("Open file")), opendir(_("Open directory")), remove(_("Remove from list")), deletedata(_("Delete file")), icon(fileInfo->getDirection()?Stock::GOTO_TOP:Stock::GOTO_BOTTOM, IconSize(ICON_SIZE_DND)), iconpause(Stock::MEDIA_PAUSE, IconSize(ICON_SIZE_BUTTON)), iconcancel(Stock::CANCEL, IconSize(ICON_SIZE_BUTTON)), iconopen(Stock::OPEN, IconSize(ICON_SIZE_BUTTON)), icondir(Stock::DIRECTORY, IconSize(ICON_SIZE_BUTTON)), icondelete(Stock::DELETE, IconSize(ICON_SIZE_BUTTON)), iconremove(Stock::REMOVE, IconSize(ICON_SIZE_BUTTON)) {
			textInfo.pack_start(fileName, PACK_EXPAND_WIDGET);
			textInfo.pack_start(peerName, PACK_EXPAND_WIDGET);
			progressContainer.pack_start(progress, PACK_EXPAND_WIDGET);
			progressContainer.pack_start(pause, PACK_SHRINK);
			progressContainer.pack_start(cancel, PACK_SHRINK);
			info.pack_start(textInfo, PACK_EXPAND_WIDGET);
			info.pack_start(icon, PACK_SHRINK);
			mainBox.pack_start(info, PACK_SHRINK);
			pause.set_image(iconpause);
			cancel.set_image(iconcancel);
			open.set_image(iconopen);
			opendir.set_image(icondir);
			remove.set_image(iconremove);
			deletedata.set_image(icondelete);
			buttonContainer.pack_start(open, PACK_SHRINK);
			buttonContainer.pack_start(opendir, PACK_SHRINK);
			buttonContainer.pack_start(remove, PACK_SHRINK);
			buttonContainer.pack_start(deletedata, PACK_SHRINK);
		mainBox.pack_start(progressContainer, PACK_SHRINK);
			updateData();
		}

		void updateData() {
			fileName.set_markup("<b>" + Markup::escape_text(getFileName(finfo->getFileName().c_str())) + "</b>");
			fileName.set_justify(JUSTIFY_LEFT);
			peerName.set_markup("<small>" + Markup::escape_text(finfo->getPeerName()) + "</small>");
			peerName.set_justify(JUSTIFY_LEFT);
			switch(finfo->getStatus()) {
				case 1:
					if(lastStatus != 1) {
						progress.set_text(_("Connecting"));
						lastStatus = 1;
					}
					progress.pulse();
					break;
				case 2:
					if(lastStatus != 2) {
						progress.set_text(_("Waiting for acceptance"));
						lastStatus = 2;
					}
					progress.pulse();
					break;
				case 3:
					progress.set_text(ustring(_("Sending")) + " (" + intToStr(finfo->getProgress()) + "%)");
					progress.set_fraction((float)finfo->getProgress() / 100);
					break;
				case 4:
					progress.set_text(ustring(_("Receiving")) + " (" + intToStr(finfo->getProgress()) + "%)");
					progress.set_fraction((float)finfo->getProgress() / 100);
					break;
				case 5:
					if(lastStatus != 5) {
						progress.set_text(ustring(_("Paused")) + " (" + intToStr(finfo->getProgress()) + "%)");
						lastStatus = 5;
					}
					progress.set_fraction((float)finfo->getProgress() / 100);
					break;
				case 6:
					if(lastStatus != 6) {
						progressContainer.remove(progress);
						progressContainer.remove(pause);
						progressContainer.remove(cancel);
						progressContainer.pack_start(buttonContainer, PACK_EXPAND_PADDING);
						lastStatus = 6;
					}
					break;
				
			}
			/*mainBox.queue_draw();*/
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
			mutex->get();
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
			finfo.addObserver(*finfoRenderer.get());
			finfoRenderer.release();
			allBox.queue_draw();

			finfoRenderer = auto_ptr<SimpleFileInfoRenderer>(new SimpleFileInfoRenderer(&finfo));
			if(direction) {
				sendBox.pack_start(finfoRenderer->getWidget(), PACK_SHRINK);
				finfo.addObserver(*finfoRenderer.get());
				finfoRenderer.release();
				++sendFileCount;
				sendBox.queue_draw();
			} else {
				recvBox.pack_start(finfoRenderer->getWidget(), PACK_SHRINK);
				finfo.addObserver(*finfoRenderer.get());
				finfoRenderer.release();
				++recvFileCount;
				recvBox.queue_draw();
			}
			show_all_children();
			trIcon.statusChanged();
			mutex->release();
		}

		inline void updateBytes(unsigned int id, uint64_t bytes) {
			mutex->get();

			std::map<unsigned int, FileInfoItem *>::iterator fileit = files.find(id);
			if(fileit != files.end()) {
				FileInfoItem *file = fileit->second;
				file->setBytes(bytes);
			}

			mutex->release();
		}

		void fileEnded(uint32_t id, uint32_t status) {
			mutex->get();
			std::map<unsigned int, FileInfoItem *>::iterator fileit = files.find(id);
			// Ignore unknown files
			if(fileit == files.end()) {
				mutex->release();
				return;
			}

			FileInfoItem &file = *fileit->second;

			if(status != 1) { // File finished TODO dont't ignore errors/cancel
				mutex->release();
				return;
			}
			file.setStatus(6);

			// In case we didn't receive update bytes
			file.setBytes(file.getSize());

			for(FileInfoItem::obsiterator it = file.firstObserver(); it != file.lastObserver(); ++it) {
				SimpleFileInfoRenderer *fiRenderer = dynamic_cast<SimpleFileInfoRenderer *>(*it);
				if(!fiRenderer) continue;

				Widget & itemwidget = fiRenderer->getWidget();
				allBox.remove(itemwidget);
				if(file.getDirection()) {
					sendBox.remove(itemwidget);
				} else {
					recvBox.remove(itemwidget);
				}

				delete fiRenderer;
			}

			if(file.getDirection()) {
				--sendFileCount;
			} else {
				--recvFileCount;
			}

			received[id] = &file;
			files.erase(fileit);

			file.clearObservers();
			auto_ptr<SimpleFileInfoRenderer> finfoRenderer(new SimpleFileInfoRenderer(&file));
			recvedBox.pack_start(finfoRenderer->getWidget(), PACK_SHRINK);
			finfoRenderer->getWidget().show_now();

			file.addObserver(*finfoRenderer);
			finfoRenderer->updateData();
			finfoRenderer.release();

			recvedBox.queue_draw();
			scrollWindowRecved.queue_draw();
			scrollWindowRecved.show_all();

			show_all_children();
			trIcon.statusChanged();

			mutex->release();
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

		virtual void sendFiles() {
#ifndef WINDOWS
			pid_t pid = fork();
			if(!pid) {
				exit(system("zenity --file-selection --multiple --title=InstantSend --separator='\n' | xargs -d '\\n' isend-gtk"));
			}
#endif
		}

		virtual void preferences() {
#ifndef WINDOWS
			pid_t pid = fork();
			if(!pid) {
				exit(system("instsend-config-wizard --conf-server"));
			}
#endif
		}

		virtual void startServer() {
			system("instsendd-wrapper start");
		}

		virtual void stopServer() {
			system("instsendd-wrapper stop");
		}

		virtual bool serverRunning() {
			return !system("instsendd-wrapper check"); // Kind of stupid; TODO: improve
		}

		virtual bool isVisible() {
			return Window::is_visible();
		}

		virtual unsigned int recvInProgress() {
			return recvFileCount;
		}

		virtual unsigned int sendInProgress() {
			return sendFileCount;
		}

		virtual unsigned int recvPending() {
			return pending.size();
		}

		virtual unsigned int sendPending() {
			return 0;
		}

	protected:
		auto_ptr<mutex_t> mutex;

		ScrolledWindow scrollWindowAll, scrollWindowRecv, scrollWindowSend, scrollWindowPending, scrollWindowRecved;
		VBox allBox, pendingBox, recvBox, sendBox, recvedBox;

		std::map<unsigned int, FileInfoItem *> files, pending, received;

		Notebook notebook;

		DBusError dberr;
		DBusConnection *dbconn;
		gtkTrayIcon trIcon;

		unsigned int sendFileCount, recvFileCount;


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
			fprintf (stderr, _("Error getting message args: %s"), dberr.message);
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
			fprintf (stderr, _("Error getting message args: %s"), dberr.message);
			dbus_error_free (&dberr);
		} else {
			isWidget.updateBytes(fileId, transferredBytes);
//			printf("Received onUpdate signal, file: %u, bytes: %lu", fileId, transferredBytes);

			handled = TRUE;
		}
	} else if(dbus_message_is_signal (message, "sk.pixelcomp.instantsend", "onEnd")) {
		DBusError dberr;
		uint32_t status;
		dbus_error_init (&dberr);
		dbus_message_get_args (message, &dberr, DBUS_TYPE_UINT32, &fileId, DBUS_TYPE_UINT32, &status, DBUS_TYPE_INVALID);

		if (dbus_error_is_set (&dberr)) {
			fprintf (stderr, _("Error getting message args: %s"), dberr.message);
			dbus_error_free (&dberr);
		} else {
			isWidget.fileEnded(fileId, status);
			handled = TRUE;
		}
	}

	return (handled ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

instSendWidget::instSendWidget() : mutex(mutex_t::getNew()), trIcon(this), sendFileCount(0), recvFileCount(0) {
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
			printf(_("Excepion occured during loading of icon: %s\n"), e.what());
		}
		catch(...) {
		}
	}

	set_title(_("InstantSend - Files"));
	set_default_size(400, 200);

	dbus_error_init (&dberr);
	dbconn = dbus_bus_get (DBUS_BUS_SESSION, &dberr);
	if (dbus_error_is_set (&dberr)) {
		fprintf (stderr, _("getting session bus failed: %s\n"), dberr.message);
		dbus_error_free (&dberr);
		return;
	}

	dbus_bus_request_name (dbconn, "sk.pixelcomp.instantsend",
	                       DBUS_NAME_FLAG_REPLACE_EXISTING, &dberr);
	if (dbus_error_is_set (&dberr)) {
		fprintf (stderr, _("requesting name failed: %s\n"), dberr.message);
		dbus_error_free (&dberr);
		return;
	}

	if (!dbus_connection_add_filter (dbconn, filter_func, this, NULL))
		return;

	dbus_bus_add_match (dbconn,
	                    "type='signal',interface='sk.pixelcomp.instantsend'",
	                    &dberr);

	if (dbus_error_is_set (&dberr)) {
		fprintf (stderr, _("Could not match: %s"), dberr.message);
		dbus_error_free (&dberr);
		return;
	}

	mutex->get();

	add(notebook);
	notebook.append_page(scrollWindowPending, _("Pending files"));
	notebook.append_page(scrollWindowAll, _("Files in progress"));
	notebook.append_page(scrollWindowRecv, _("Incoming files"));
	notebook.append_page(scrollWindowSend, _("Outgoing files"));
	notebook.append_page(scrollWindowRecved, _("Received files"));

	scrollWindowPending.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrollWindowAll.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrollWindowRecv.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrollWindowSend.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrollWindowRecved.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);

	scrollWindowAll.add(allBox);
	scrollWindowRecv.add(recvBox);
	scrollWindowSend.add(sendBox);
	scrollWindowPending.add(pendingBox);
	scrollWindowRecved.add(recvedBox);

	show_all_children();

	mutex->release();

	signal_timeout().connect(sigc::mem_fun(*this, &instSendWidget::on_timeout), 100);
	signal_timeout().connect(sigc::mem_fun(*this, &instSendWidget::on_icon_timeout), 200);
}

int main(int argc, char *argv[]) {
	setlocale(LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	textdomain(GETTEXT_PACKAGE);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	Main app(argc, argv, "sk.pixelcomp.instantsend.widget");
	appPtr = &app;
	instSendWidget window;
	app.run();
	return 0;
}
